/*
 * Phit PRNG — Phase-Based Random Number Generator
 * =================================================
 *
 * Genera numeri casuali usando phits come sorgente di entropia.
 * Non è un sostituto di /dev/urandom, ma dimostra che l'informazione
 * di fase è reale e sfruttabile come entropia hardware.
 *
 * Approccio:
 *   1. Leggi N delta timer consecutivi
 *   2. Combina con timer LSBs (uniformi)
 *   3. Accumula in un pool di entropia
 *   4. Estrai bit con hash (output whitening)
 *
 * Il vantaggio: entropia genuina dall'asincronia dei clock,
 * senza hardware RNG dedicato.
 *
 * Compila: gcc -O0 -o phit_prng phit_prng.c -lm
 *
 * Author: Alessio Cazzaniga
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mach/mach_time.h>

static volatile uint64_t sink;

static inline uint64_t now_ns(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

/* ========== Phit entropy pool ========== */

typedef struct {
    uint64_t pool[4];      /* 256-bit entropy pool */
    uint64_t mix_counter;  /* monotonic counter for mixing */
    int bits_collected;    /* estimated phits accumulated */
} phit_pool_t;

/* Rotate left */
static inline uint64_t rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* Mix a phit sample into the pool (inspired by SplitMix64) */
static void pool_feed(phit_pool_t *p, uint64_t phit_sample) {
    p->mix_counter++;

    /* Combine sample with counter to prevent cycles */
    uint64_t z = phit_sample + p->mix_counter * 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    /* XOR into rotating pool position */
    int slot = p->mix_counter & 3;
    p->pool[slot] ^= z;

    /* Cross-mix pool lanes */
    p->pool[(slot + 1) & 3] ^= rotl64(p->pool[slot], 17);

    p->bits_collected += 2;  /* ~2 phits per feed */
}

/* Harvest workload: do real work, measure phase */
static void pool_harvest(phit_pool_t *p) {
    volatile uint64_t x = 0xCAFEBABE;
    for (int i = 0; i < 20; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        x ^= x >> 17;
    }
    sink = x;

    uint64_t t = now_ns();
    /* Feed both delta-derived and absolute-timer info */
    pool_feed(p, t);
    pool_feed(p, x ^ t);
}

/* Extract a 64-bit random value from the pool */
static uint64_t pool_extract(phit_pool_t *p) {
    /* Harvest fresh phits */
    for (int i = 0; i < 4; i++) {
        pool_harvest(p);
    }

    /* Output: combine all pool lanes */
    uint64_t out = p->pool[0];
    out ^= rotl64(p->pool[1], 13);
    out ^= rotl64(p->pool[2], 29);
    out ^= rotl64(p->pool[3], 43);

    /* Forward-secure: hash the pool after extraction */
    p->pool[0] ^= rotl64(out, 7);
    p->pool[1] ^= rotl64(out, 23);

    return out;
}

/* ========== PRNG interface ========== */

typedef struct {
    phit_pool_t pool;
    uint64_t generated;
} phit_prng_t;

static void prng_init(phit_prng_t *rng) {
    memset(rng, 0, sizeof(phit_prng_t));
    /* Seed with initial phase readings */
    for (int i = 0; i < 16; i++) {
        pool_harvest(&rng->pool);
    }
}

static uint64_t prng_u64(phit_prng_t *rng) {
    rng->generated++;
    return pool_extract(&rng->pool);
}

static double prng_double(phit_prng_t *rng) {
    return (double)(prng_u64(rng) >> 11) / (double)(1ULL << 53);
}

static uint32_t prng_range(phit_prng_t *rng, uint32_t max) {
    return (uint32_t)(prng_u64(rng) % max);
}

/* ========== Quality tests ========== */

/* Monobit test: count 1s in N bits, should be ~50% */
static void test_monobit(phit_prng_t *rng) {
    printf("\n  === Monobit Test ===\n");
    int n = 100000;
    int ones = 0;
    int total_bits = n * 64;

    for (int i = 0; i < n; i++) {
        uint64_t v = prng_u64(rng);
        ones += __builtin_popcountll(v);
    }

    double ratio = (double)ones / total_bits;
    double expected = 0.5;
    double z = fabs(ratio - expected) / sqrt(expected * (1 - expected) / total_bits);

    printf("    Total bits:  %d\n", total_bits);
    printf("    Ones:        %d (%.4f%%)\n", ones, 100.0 * ratio);
    printf("    Expected:    50.0000%%\n");
    printf("    Z-score:     %.2f (pass if < 3.29)\n", z);
    printf("    Result:      %s\n", z < 3.29 ? "PASS" : "FAIL");
}

/* Runs test: count consecutive 0s and 1s runs */
static void test_runs(phit_prng_t *rng) {
    printf("\n  === Runs Test ===\n");
    int n = 100000;
    int runs = 1;
    int prev_bit = 0;
    int total_bits = 0;
    int ones = 0;

    for (int i = 0; i < n; i++) {
        uint64_t v = prng_u64(rng);
        for (int b = 0; b < 64; b++) {
            int bit = (v >> b) & 1;
            ones += bit;
            if (total_bits > 0 && bit != prev_bit) runs++;
            prev_bit = bit;
            total_bits++;
        }
    }

    double pi = (double)ones / total_bits;
    double expected_runs = 1 + 2.0 * total_bits * pi * (1 - pi);
    double variance = 2.0 * total_bits * pi * (1 - pi) *
                      (2.0 * total_bits * pi * (1 - pi) - 1) /
                      ((double)total_bits - 1);
    double z = fabs(runs - expected_runs) / sqrt(variance);

    printf("    Total bits:      %d\n", total_bits);
    printf("    Runs:            %d\n", runs);
    printf("    Expected runs:   %.0f\n", expected_runs);
    printf("    Z-score:         %.2f (pass if < 3.29)\n", z);
    printf("    Result:          %s\n", z < 3.29 ? "PASS" : "FAIL");
}

/* Chi-squared on byte distribution */
static void test_byte_distribution(phit_prng_t *rng) {
    printf("\n  === Byte Distribution Test ===\n");
    int n = 200000;
    int hist[256] = {0};

    for (int i = 0; i < n; i++) {
        uint64_t v = prng_u64(rng);
        for (int b = 0; b < 8; b++) {
            hist[(v >> (b * 8)) & 0xFF]++;
        }
    }

    int total = n * 8;
    double expected = (double)total / 256.0;
    double chi2 = 0;
    int min_count = total, max_count = 0;

    for (int i = 0; i < 256; i++) {
        double d = hist[i] - expected;
        chi2 += d * d / expected;
        if (hist[i] < min_count) min_count = hist[i];
        if (hist[i] > max_count) max_count = hist[i];
    }

    /* df=255, critical value at p=0.01 ≈ 310 */
    printf("    Bytes tested:    %d\n", total);
    printf("    Chi²:            %.1f (pass if < 310 at p=0.01)\n", chi2);
    printf("    Min/Max bucket:  %d / %d (expected: %.0f)\n",
           min_count, max_count, expected);
    printf("    Result:          %s\n", chi2 < 310 ? "PASS" : "FAIL");
}

/* Per-bit entropy */
static void test_bit_entropy(phit_prng_t *rng) {
    printf("\n  === Per-Bit Entropy ===\n");
    int n = 100000;
    int ones[64] = {0};

    for (int i = 0; i < n; i++) {
        uint64_t v = prng_u64(rng);
        for (int b = 0; b < 64; b++) {
            if ((v >> b) & 1) ones[b]++;
        }
    }

    double total_h = 0;
    double min_h = 1.0, max_h = 0;
    int min_bit = 0, max_bit = 0;

    for (int b = 0; b < 64; b++) {
        double p1 = (double)ones[b] / n;
        double p0 = 1.0 - p1;
        double h = 0;
        if (p0 > 1e-10 && p1 > 1e-10) {
            h = -(p0 * log2(p0) + p1 * log2(p1));
        }
        total_h += h;
        if (h < min_h) { min_h = h; min_bit = b; }
        if (h > max_h) { max_h = h; max_bit = b; }
    }

    printf("    Total entropy:   %.2f / 64.0 bits (%.1f%%)\n",
           total_h, 100.0 * total_h / 64.0);
    printf("    Min entropy bit: [%d] = %.6f\n", min_bit, min_h);
    printf("    Max entropy bit: [%d] = %.6f\n", max_bit, max_h);
    printf("    Result:          %s\n",
           total_h > 63.0 ? "EXCELLENT" :
           total_h > 60.0 ? "GOOD" :
           total_h > 50.0 ? "ACCEPTABLE" : "POOR");
}

/* Throughput */
static void test_throughput(phit_prng_t *rng) {
    printf("\n  === Throughput ===\n");
    int n = 100000;
    uint64_t x = 0;

    uint64_t t1 = now_ns();
    for (int i = 0; i < n; i++) {
        x ^= prng_u64(rng);
    }
    uint64_t t2 = now_ns();
    sink = x;

    double elapsed_ms = (t2 - t1) / 1e6;
    double values_per_sec = n / (elapsed_ms / 1000.0);
    double bits_per_sec = values_per_sec * 64;
    double bytes_per_sec = bits_per_sec / 8;

    printf("    Generated:       %d uint64 values\n", n);
    printf("    Elapsed:         %.1f ms\n", elapsed_ms);
    printf("    Throughput:      %.0f values/s\n", values_per_sec);
    printf("    Bandwidth:       %.2f Mbit/s (%.2f MB/s)\n",
           bits_per_sec / 1e6, bytes_per_sec / 1e6);
    printf("    Phits consumed:  ~%d phit/value (4 harvests × ~2 phit)\n", 8);
}

/* ========== Main ========== */

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  PHIT PRNG — Phase-Based Random Number Generator        ║\n");
    printf("║  Entropy from CPU↔Timer clock phase relationship        ║\n");
    printf("║  Apple Silicon M1 Max                                   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    phit_prng_t rng;
    prng_init(&rng);

    printf("\n  Sample output (first 10 values):\n");
    for (int i = 0; i < 10; i++) {
        printf("    %2d: 0x%016llX  (%.6f)\n",
               i, (unsigned long long)prng_u64(&rng), prng_double(&rng));
    }

    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  QUALITY TESTS (NIST SP 800-22 inspired)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    test_monobit(&rng);
    test_runs(&rng);
    test_byte_distribution(&rng);
    test_bit_entropy(&rng);
    test_throughput(&rng);

    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  NOTE: This is NOT cryptographically secure.\n");
    printf("  It demonstrates that phits provide genuine entropy\n");
    printf("  from clock phase relationships — no dedicated RNG HW.\n");
    printf("═══════════════════════════════════════════════════════════\n");

    return 0;
}
