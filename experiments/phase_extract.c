/*
 * Phase Information Extraction — Apple Silicon (M1 Max)
 * =====================================================
 *
 * Misura l'informazione di fase estraibile dal rapporto tra
 * il timer ARM (cntvct_el0, 24 MHz) e il clock CPU.
 *
 * Compila: gcc -O0 -o phase_extract phase_extract.c -lm
 * Esegui:  ./phase_extract
 *
 * Author: Alessio Cazzaniga
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define NUM_SAMPLES   100000
#define NUM_BINS      256
#define MAX_LAG       200

/* ---------- Timer read ---------- */

static inline uint64_t read_timer(void) {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else
    #error "Unsupported architecture"
#endif
}

/* ---------- Workloads ---------- */

static volatile uint64_t sink;

static void workload_nop(void) {
    __asm__ volatile("nop");
}

static void workload_alu(void) {
    volatile uint64_t x = 0x12345678;
    for (int i = 0; i < 10; i++) {
        x = x * 0x5DEECE66DUL + 0xBUL;
    }
    sink = x;
}

static void workload_memory(void) {
    static volatile char buf[4096];
    for (int i = 0; i < 64; i += 16) {
        buf[i] = (char)(i & 0xFF);
    }
    sink = buf[0];
}

/* ---------- Experiment 1: Raw delta distribution ---------- */

static void experiment_raw_distribution(void) {
    printf("\n=== EXPERIMENT 1: Raw Delta Distribution ===\n");

    uint64_t deltas[NUM_SAMPLES];
    uint64_t min_d = UINT64_MAX, max_d = 0;
    double sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        uint64_t t1 = read_timer();
        workload_nop();
        uint64_t t2 = read_timer();
        deltas[i] = t2 - t1;
        if (deltas[i] < min_d) min_d = deltas[i];
        if (deltas[i] > max_d) max_d = deltas[i];
        sum += deltas[i];
    }

    double mean = sum / NUM_SAMPLES;
    double var = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double d = deltas[i] - mean;
        var += d * d;
    }
    var /= NUM_SAMPLES;

    printf("  Samples:  %d\n", NUM_SAMPLES);
    printf("  Min:      %llu ticks\n", (unsigned long long)min_d);
    printf("  Max:      %llu ticks\n", (unsigned long long)max_d);
    printf("  Mean:     %.2f ticks\n", mean);
    printf("  Std:      %.2f ticks\n", sqrt(var));
    printf("  Range:    %llu ticks\n", (unsigned long long)(max_d - min_d));

    /* Histogram of LSBs */
    int hist[NUM_BINS] = {0};
    for (int i = 0; i < NUM_SAMPLES; i++) {
        hist[deltas[i] & 0xFF]++;
    }

    printf("\n  LSB byte histogram (top 10):\n");
    /* Find top 10 bins */
    for (int rank = 0; rank < 10; rank++) {
        int best_bin = 0, best_count = 0;
        for (int b = 0; b < NUM_BINS; b++) {
            if (hist[b] > best_count) {
                best_count = hist[b];
                best_bin = b;
            }
        }
        if (best_count == 0) break;
        printf("    0x%02X (%3d): %d (%.1f%%)\n",
               best_bin, best_bin, best_count,
               100.0 * best_count / NUM_SAMPLES);
        hist[best_bin] = 0;
    }
}

/* ---------- Experiment 2: Per-bit entropy ---------- */

static void experiment_bit_entropy(void) {
    printf("\n=== EXPERIMENT 2: Per-Bit Entropy (Shannon) ===\n");

    uint64_t deltas[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        uint64_t t1 = read_timer();
        workload_alu();
        uint64_t t2 = read_timer();
        deltas[i] = t2 - t1;
    }

    printf("  Bit | P(1)   | Entropy (max=1.0)\n");
    printf("  ----|--------|------------------\n");

    double total_entropy = 0;
    for (int bit = 0; bit < 16; bit++) {
        int ones = 0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            if ((deltas[i] >> bit) & 1) ones++;
        }
        double p1 = (double)ones / NUM_SAMPLES;
        double p0 = 1.0 - p1;
        double h = 0;
        if (p0 > 0 && p1 > 0) {
            h = -(p0 * log2(p0) + p1 * log2(p1));
        }
        total_entropy += h;
        printf("   %2d | %.4f | %.4f %s\n", bit, p1, h,
               h > 0.9 ? "***" : h > 0.5 ? "**" : h > 0.1 ? "*" : "");
    }
    printf("  Total extractable entropy: %.2f bits\n", total_entropy);
}

/* ---------- Experiment 3: Autocorrelation ---------- */

static void experiment_autocorrelation(void) {
    printf("\n=== EXPERIMENT 3: Autocorrelation ===\n");

    int n = 10000;
    uint64_t deltas[10000];
    double mean = 0;

    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_timer();
        workload_alu();
        uint64_t t2 = read_timer();
        deltas[i] = t2 - t1;
        mean += deltas[i];
    }
    mean /= n;

    double var = 0;
    for (int i = 0; i < n; i++) {
        double d = deltas[i] - mean;
        var += d * d;
    }
    var /= n;

    printf("  Lag | Autocorrelation\n");
    printf("  ----|----------------\n");

    int lags[] = {1, 2, 3, 5, 10, 20, 50, 100, MAX_LAG};
    int num_lags = sizeof(lags) / sizeof(lags[0]);

    for (int li = 0; li < num_lags; li++) {
        int lag = lags[li];
        if (lag >= n) break;
        double cov = 0;
        for (int i = 0; i < n - lag; i++) {
            cov += (deltas[i] - mean) * (deltas[i + lag] - mean);
        }
        cov /= (n - lag);
        double r = (var > 0) ? cov / var : 0;

        /* Visual bar */
        char bar[52];
        memset(bar, ' ', 51);
        bar[51] = '\0';
        bar[25] = '|';
        int pos = 25 + (int)(r * 25);
        if (pos < 0) pos = 0;
        if (pos > 50) pos = 50;
        bar[pos] = '#';

        printf("  %3d | %+.4f [%s]\n", lag, r, bar);
    }
}

/* ---------- Experiment 4: Workload comparison ---------- */

static void experiment_workload_comparison(void) {
    printf("\n=== EXPERIMENT 4: Jitter by Workload ===\n");

    struct {
        const char *name;
        void (*fn)(void);
    } workloads[] = {
        {"NOP (baseline)", workload_nop},
        {"ALU (10 muls)",  workload_alu},
        {"Memory (cache)", workload_memory},
    };
    int nw = sizeof(workloads) / sizeof(workloads[0]);

    printf("  %-20s | Mean     | Std      | Range\n", "Workload");
    printf("  --------------------|----------|----------|--------\n");

    for (int w = 0; w < nw; w++) {
        int n = 50000;
        uint64_t min_d = UINT64_MAX, max_d = 0;
        double sum = 0, sum2 = 0;

        for (int i = 0; i < n; i++) {
            uint64_t t1 = read_timer();
            workloads[w].fn();
            uint64_t t2 = read_timer();
            uint64_t d = t2 - t1;
            sum += d;
            sum2 += (double)d * d;
            if (d < min_d) min_d = d;
            if (d > max_d) max_d = d;
        }
        double mean = sum / n;
        double std = sqrt(sum2 / n - mean * mean);
        printf("  %-20s | %8.1f | %8.1f | %llu\n",
               workloads[w].name, mean, std,
               (unsigned long long)(max_d - min_d));
    }
}

/* ---------- Experiment 5: Phase-dependent branching ---------- */

static void experiment_phase_branching(void) {
    printf("\n=== EXPERIMENT 5: Phase-Dependent Branching ===\n");
    printf("  (Simulates using timer LSBs to select operations)\n\n");

    int counters[4] = {0, 0, 0, 0};
    int n = 100000;

    for (int i = 0; i < n; i++) {
        uint64_t t = read_timer();
        int slot = t & 3;  /* 2 LSBs = 4 phase slots */
        counters[slot]++;
    }

    printf("  Slot | Count  | Fraction | Ideal=25%%\n");
    printf("  -----|--------|----------|----------\n");
    for (int s = 0; s < 4; s++) {
        double frac = 100.0 * counters[s] / n;
        printf("    %d  | %6d | %5.1f%%   | %s\n",
               s, counters[s], frac,
               fabs(frac - 25.0) < 2.0 ? "OK" : "BIASED");
    }

    /* Chi-squared test */
    double chi2 = 0;
    double expected = n / 4.0;
    for (int s = 0; s < 4; s++) {
        double d = counters[s] - expected;
        chi2 += d * d / expected;
    }
    printf("\n  Chi-squared: %.2f (uniform if < 7.81 at p=0.05)\n", chi2);
    printf("  Result: %s\n", chi2 < 7.81 ? "UNIFORM - good entropy" :
                                             "NON-UNIFORM - bias detected");
}

/* ---------- Main ---------- */

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  TRIPHASE: Phase Information Extraction      ║\n");
    printf("║  Hardware: ");
#if defined(__aarch64__)
    printf("Apple Silicon (ARM64)            ║\n");
#elif defined(__x86_64__)
    printf("x86_64 (TSC)                     ║\n");
#endif
    printf("╚══════════════════════════════════════════════╝\n");

    experiment_raw_distribution();
    experiment_bit_entropy();
    experiment_autocorrelation();
    experiment_workload_comparison();
    experiment_phase_branching();

    printf("\n=== SUMMARY ===\n");
    printf("Run on Apple Silicon M1 Max for best results.\n");
    printf("Key metrics:\n");
    printf("  - Entropy > 8 bits = rich phase information\n");
    printf("  - Autocorrelation structure = exploitable patterns\n");
    printf("  - Uniform branching = usable for phase-gated ops\n");

    return 0;
}
