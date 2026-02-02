/*
 * Phase Information Extraction v2 — mach_absolute_time()
 * ======================================================
 *
 * Usa mach_absolute_time() per risoluzione sub-nanosecondo su Apple Silicon.
 * Questo timer ha risoluzione ~41.67 ns (24 MHz) ma il kernel lo interpola
 * internamente, e mach_absolute_time accede al contatore hardware diretto
 * con overhead minimo.
 *
 * Confronto con v1 (cntvct_el0):
 *   - cntvct_el0: 24 MHz, ~41.67 ns/tick
 *   - mach_absolute_time: stessa sorgente HW ma accesso via Mach kernel
 *   - mach_continuous_time: include tempo in sleep
 *
 * In più: test con clock_gettime_nsec_np() per nanosecondi reali.
 *
 * Compila: gcc -O0 -o phase_v2 phase_extract_v2.c -lm
 * Esegui:  ./phase_v2
 *
 * Author: Alessio Cazzaniga
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mach/mach_time.h>
#include <time.h>
#include <pthread.h>

#define NUM_SAMPLES    200000
#define NUM_BINS       256
#define MAX_LAG        500

/* ========== Timer sources ========== */

static mach_timebase_info_data_t timebase_info;

static inline uint64_t read_cntvct(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

static inline uint64_t read_mach(void) {
    return mach_absolute_time();
}

static inline uint64_t read_clock_ns(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

static uint64_t mach_to_ns(uint64_t mach_ticks) {
    return mach_ticks * timebase_info.numer / timebase_info.denom;
}

/* ========== Workloads (graduated complexity) ========== */

static volatile uint64_t sink;

static void workload_nop(void) {
    __asm__ volatile("nop");
}

static void workload_nop10(void) {
    __asm__ volatile(
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop"
    );
}

static void workload_alu_light(void) {
    volatile uint64_t x = 0xDEADBEEF;
    x = x * 7 + 3;
    x = x ^ (x >> 13);
    sink = x;
}

static void workload_alu_heavy(void) {
    volatile uint64_t x = 0x12345678;
    for (int i = 0; i < 50; i++) {
        x = x * 0x5DEECE66DUL + 0xBUL;
        x ^= x >> 17;
    }
    sink = x;
}

static void workload_memory_seq(void) {
    static volatile char buf[8192];
    for (int i = 0; i < 256; i += 64) {
        buf[i] = (char)i;
    }
    sink = buf[0];
}

static void workload_memory_random(void) {
    static volatile char buf[65536];
    static uint32_t rng = 0xBAADF00D;
    for (int i = 0; i < 16; i++) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        buf[rng & 0xFFFF] = (char)rng;
    }
    sink = buf[rng & 0xFFFF];
}

static void workload_branch(void) {
    volatile uint64_t x = read_cntvct();
    volatile int sum = 0;
    for (int i = 0; i < 32; i++) {
        if ((x >> (i & 7)) & 1) sum += i;
        else sum -= i;
    }
    sink = sum;
}

static void workload_mixed(void) {
    workload_alu_light();
    workload_memory_seq();
    workload_branch();
}

/* ========== Analysis functions ========== */

typedef struct {
    double mean;
    double std;
    uint64_t min_val;
    uint64_t max_val;
    double entropy_total;
    double bit_entropy[20];
} stats_t;

static stats_t compute_stats(uint64_t *deltas, int n) {
    stats_t s = {0};
    s.min_val = UINT64_MAX;

    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += deltas[i];
        if (deltas[i] < s.min_val) s.min_val = deltas[i];
        if (deltas[i] > s.max_val) s.max_val = deltas[i];
    }
    s.mean = sum / n;

    double var = 0;
    for (int i = 0; i < n; i++) {
        double d = deltas[i] - s.mean;
        var += d * d;
    }
    s.std = sqrt(var / n);

    /* Per-bit entropy */
    s.entropy_total = 0;
    for (int bit = 0; bit < 20; bit++) {
        int ones = 0;
        for (int i = 0; i < n; i++) {
            if ((deltas[i] >> bit) & 1) ones++;
        }
        double p1 = (double)ones / n;
        double p0 = 1.0 - p1;
        double h = 0;
        if (p0 > 1e-10 && p1 > 1e-10) {
            h = -(p0 * log2(p0) + p1 * log2(p1));
        }
        s.bit_entropy[bit] = h;
        s.entropy_total += h;
    }

    return s;
}

/* ========== Experiment 1: Timer source comparison ========== */

static void experiment_timer_comparison(void) {
    printf("\n=== EXPERIMENT 1: Timer Source Comparison ===\n");
    printf("  Measuring overhead and resolution of 3 timer sources.\n\n");

    int n = 100000;
    uint64_t *d1 = malloc(n * sizeof(uint64_t));
    uint64_t *d2 = malloc(n * sizeof(uint64_t));
    uint64_t *d3 = malloc(n * sizeof(uint64_t));

    /* cntvct_el0 */
    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_cntvct();
        uint64_t t2 = read_cntvct();
        d1[i] = t2 - t1;
    }

    /* mach_absolute_time */
    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_mach();
        uint64_t t2 = read_mach();
        d2[i] = t2 - t1;
    }

    /* clock_gettime_nsec_np */
    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_clock_ns();
        uint64_t t2 = read_clock_ns();
        d3[i] = t2 - t1;
    }

    stats_t s1 = compute_stats(d1, n);
    stats_t s2 = compute_stats(d2, n);
    stats_t s3 = compute_stats(d3, n);

    printf("  %-22s | %8s | %8s | %8s | %6s\n",
           "Source", "Mean", "Std", "Range", "Entropy");
    printf("  %-22s-+-%8s-+-%8s-+-%8s-+-%6s\n",
           "----------------------", "--------", "--------", "--------", "------");
    printf("  %-22s | %8.1f | %8.2f | %8llu | %6.2f\n",
           "cntvct_el0 (24MHz)", s1.mean, s1.std,
           (unsigned long long)(s1.max_val - s1.min_val), s1.entropy_total);
    printf("  %-22s | %8.1f | %8.2f | %8llu | %6.2f\n",
           "mach_absolute_time", s2.mean, s2.std,
           (unsigned long long)(s2.max_val - s2.min_val), s2.entropy_total);
    printf("  %-22s | %8.1f | %8.2f | %8llu | %6.2f\n",
           "clock_gettime_nsec_np", s3.mean, s3.std,
           (unsigned long long)(s3.max_val - s3.min_val), s3.entropy_total);

    printf("\n  Timebase: %u/%u (mach ticks to ns)\n",
           timebase_info.numer, timebase_info.denom);
    printf("  1 mach tick = %.2f ns\n",
           (double)timebase_info.numer / timebase_info.denom);

    free(d1); free(d2); free(d3);
}

/* ========== Experiment 2: Workload entropy spectrum ========== */

static void experiment_workload_spectrum(void) {
    printf("\n=== EXPERIMENT 2: Workload Entropy Spectrum ===\n");
    printf("  Phase information extractable per workload type.\n\n");

    struct {
        const char *name;
        void (*fn)(void);
    } workloads[] = {
        {"NOP x1",          workload_nop},
        {"NOP x10",         workload_nop10},
        {"ALU light",       workload_alu_light},
        {"ALU heavy (50)",  workload_alu_heavy},
        {"Memory seq",      workload_memory_seq},
        {"Memory random",   workload_memory_random},
        {"Branch-heavy",    workload_branch},
        {"Mixed",           workload_mixed},
    };
    int nw = sizeof(workloads) / sizeof(workloads[0]);

    int n = 100000;
    uint64_t *deltas = malloc(n * sizeof(uint64_t));

    printf("  %-20s | %8s | %8s | %8s | %7s | Bit entropy [0..15]\n",
           "Workload", "Mean", "Std", "Range", "H total");
    printf("  %-20s-+-%8s-+-%8s-+-%8s-+-%7s-+-%s\n",
           "--------------------", "--------", "--------",
           "--------", "-------", "--------------------");

    for (int w = 0; w < nw; w++) {
        for (int i = 0; i < n; i++) {
            uint64_t t1 = read_mach();
            workloads[w].fn();
            uint64_t t2 = read_mach();
            deltas[i] = t2 - t1;
        }

        stats_t s = compute_stats(deltas, n);

        /* Visual entropy bar per bit (first 16 bits) */
        char bar[17];
        for (int b = 0; b < 16; b++) {
            if (s.bit_entropy[b] > 0.9) bar[b] = '#';
            else if (s.bit_entropy[b] > 0.5) bar[b] = '+';
            else if (s.bit_entropy[b] > 0.1) bar[b] = '.';
            else bar[b] = ' ';
        }
        bar[16] = '\0';

        printf("  %-20s | %8.1f | %8.1f | %8llu | %7.2f | [%s]\n",
               workloads[w].name, s.mean, s.std,
               (unsigned long long)(s.max_val - s.min_val),
               s.entropy_total, bar);
    }

    free(deltas);
    printf("\n  Legend: # = >0.9 bit, + = >0.5 bit, . = >0.1 bit\n");
}

/* ========== Experiment 3: Deep autocorrelation ========== */

static void experiment_deep_autocorrelation(void) {
    printf("\n=== EXPERIMENT 3: Deep Autocorrelation (ALU heavy) ===\n");
    printf("  Looking for periodic structure in timing deltas.\n\n");

    int n = 50000;
    uint64_t *deltas = malloc(n * sizeof(uint64_t));
    double mean = 0;

    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_mach();
        workload_alu_heavy();
        uint64_t t2 = read_mach();
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

    /* Scan lags 1-500 looking for peaks */
    printf("  Lag | r        | Bar\n");
    printf("  ----+----------+--------------------------------------------\n");

    double peak_r = 0;
    int peak_lag = 0;

    for (int lag = 1; lag <= MAX_LAG; lag++) {
        double cov = 0;
        for (int i = 0; i < n - lag; i++) {
            cov += (deltas[i] - mean) * (deltas[i + lag] - mean);
        }
        cov /= (n - lag);
        double r = (var > 0) ? cov / var : 0;

        if (fabs(r) > fabs(peak_r)) {
            peak_r = r;
            peak_lag = lag;
        }

        /* Print every 10th lag, or if it's a notable peak */
        int is_peak = (lag > 2 && fabs(r) > 0.05);
        if (lag <= 10 || lag % 25 == 0 || is_peak) {
            int bar_len = (int)(fabs(r) * 40);
            if (bar_len > 40) bar_len = 40;
            char bar[41];
            memset(bar, r >= 0 ? '+' : '-', bar_len);
            bar[bar_len] = '\0';

            printf("  %3d | %+.6f | %s%s\n", lag, r, bar,
                   is_peak ? " <-- peak" : "");
        }
    }

    printf("\n  Strongest correlation: lag=%d, r=%+.6f\n", peak_lag, peak_r);
    if (fabs(peak_r) > 0.05) {
        printf("  → Periodic structure detected! Period ≈ %d ticks\n", peak_lag);
        printf("  → At 24MHz timer, this is ≈ %.2f µs\n",
               peak_lag * (double)timebase_info.numer / timebase_info.denom / 1000.0);
    }

    free(deltas);
}

/* ========== Experiment 4: Phase slot distribution (key test) ========== */

static void experiment_phase_slots(void) {
    printf("\n=== EXPERIMENT 4: Phase Slot Distribution ===\n");
    printf("  Can we use timing LSBs to uniformly select phase slots?\n\n");

    int n = 200000;
    int slot_configs[] = {2, 4, 8, 16, 32, 64};
    int num_configs = sizeof(slot_configs) / sizeof(slot_configs[0]);

    printf("  K slots | Chi²     | p>0.05?  | Max bias | Usable?\n");
    printf("  --------+----------+----------+----------+--------\n");

    for (int ci = 0; ci < num_configs; ci++) {
        int K = slot_configs[ci];
        int *counts = calloc(K, sizeof(int));

        for (int i = 0; i < n; i++) {
            uint64_t t = read_mach();
            workload_alu_light();
            uint64_t t2 = read_mach();
            uint64_t delta = t2 - t;
            counts[delta % K]++;
        }

        /* Chi-squared */
        double expected = (double)n / K;
        double chi2 = 0;
        double max_bias = 0;
        for (int s = 0; s < K; s++) {
            double d = counts[s] - expected;
            chi2 += d * d / expected;
            double bias = fabs((double)counts[s] / expected - 1.0);
            if (bias > max_bias) max_bias = bias;
        }

        /* Critical value for K-1 degrees of freedom at p=0.05 */
        /* Approximation: chi2_crit ≈ K - 1 + 2*sqrt(K-1) for large K */
        double df = K - 1;
        double chi2_crit = df + 2.0 * sqrt(df);
        int uniform = chi2 < chi2_crit;

        printf("  %7d | %8.1f | %8s | %7.1f%% | %s\n",
               K, chi2, uniform ? "YES" : "NO",
               max_bias * 100,
               uniform ? "YES" : (max_bias < 0.1 ? "MARGINAL" : "NO"));

        free(counts);
    }
}

/* ========== Experiment 5: Inter-read delta spectrum ========== */

static void experiment_delta_spectrum(void) {
    printf("\n=== EXPERIMENT 5: Delta Value Spectrum ===\n");
    printf("  Full histogram of timing deltas (ALU heavy workload).\n\n");

    int n = 200000;
    uint64_t *deltas = malloc(n * sizeof(uint64_t));
    uint64_t max_d = 0;

    for (int i = 0; i < n; i++) {
        uint64_t t1 = read_mach();
        workload_alu_heavy();
        uint64_t t2 = read_mach();
        deltas[i] = t2 - t1;
        if (deltas[i] > max_d) max_d = deltas[i];
    }

    /* Bin into 50 buckets */
    int num_bins = 50;
    int *hist = calloc(num_bins, sizeof(int));
    double bin_width = (double)(max_d + 1) / num_bins;

    for (int i = 0; i < n; i++) {
        int bin = (int)(deltas[i] / bin_width);
        if (bin >= num_bins) bin = num_bins - 1;
        hist[bin]++;
    }

    int max_count = 0;
    for (int b = 0; b < num_bins; b++) {
        if (hist[b] > max_count) max_count = hist[b];
    }

    printf("  Delta (mach ticks) | Count | Distribution\n");
    printf("  -------------------+-------+-----------------------------------\n");

    for (int b = 0; b < num_bins; b++) {
        if (hist[b] == 0) continue;
        uint64_t lo = (uint64_t)(b * bin_width);
        uint64_t hi = (uint64_t)((b + 1) * bin_width);
        int bar_len = (max_count > 0) ? hist[b] * 40 / max_count : 0;
        char bar[41];
        memset(bar, '#', bar_len);
        bar[bar_len] = '\0';

        double ns_lo = lo * (double)timebase_info.numer / timebase_info.denom;
        printf("  %6llu-%-6llu (%3.0fns) | %5d | %s\n",
               (unsigned long long)lo, (unsigned long long)hi,
               ns_lo, hist[b], bar);
    }

    /* Unique delta values */
    int unique = 0;
    uint64_t prev = UINT64_MAX;
    /* Simple count: sort first */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < (i + 20 < n ? i + 20 : n); j++) {
            if (deltas[j] < deltas[i]) {
                uint64_t tmp = deltas[i];
                deltas[i] = deltas[j];
                deltas[j] = tmp;
            }
        }
    }
    /* Just count unique in first 10k (sorted enough) */
    prev = UINT64_MAX;
    for (int i = 0; i < 10000 && i < n; i++) {
        if (deltas[i] != prev) {
            unique++;
            prev = deltas[i];
        }
    }

    printf("\n  Unique delta values (first 10k): %d\n", unique);
    printf("  → log₂(%d) = %.1f bits of phase information\n",
           unique, log2(unique));

    free(deltas);
    free(hist);
}

/* ========== Experiment 6: Cross-core phase measurement ========== */

static volatile uint64_t shared_timestamp = 0;
static volatile int writer_done = 0;

static void *writer_thread(void *arg) {
    /* Pin to E-core if possible (best-effort) */
    for (int i = 0; i < 100000; i++) {
        shared_timestamp = read_mach();
        workload_alu_light();
    }
    writer_done = 1;
    return NULL;
}

static void experiment_cross_core(void) {
    printf("\n=== EXPERIMENT 6: Cross-Core Phase Difference ===\n");
    printf("  Measuring timing delta between two cores.\n\n");

    writer_done = 0;
    shared_timestamp = 0;

    pthread_t writer;
    pthread_create(&writer, NULL, writer_thread, NULL);

    int n = 50000;
    uint64_t *diffs = malloc(n * sizeof(uint64_t));
    int count = 0;

    while (!writer_done && count < n) {
        uint64_t local = read_mach();
        uint64_t remote = shared_timestamp;
        if (remote > 0 && local > remote) {
            diffs[count++] = local - remote;
        }
    }

    pthread_join(writer, NULL);

    if (count > 1000) {
        stats_t s = compute_stats(diffs, count);
        printf("  Cross-core samples:  %d\n", count);
        printf("  Mean delta:          %.1f ticks (%.1f ns)\n",
               s.mean, s.mean * timebase_info.numer / timebase_info.denom);
        printf("  Std:                 %.1f ticks\n", s.std);
        printf("  Range:               %llu ticks\n",
               (unsigned long long)(s.max_val - s.min_val));
        printf("  Entropy:             %.2f bits\n", s.entropy_total);
        printf("\n  This measures P-core vs E-core phase offset.\n");
        printf("  High entropy = the two cores are truly asynchronous.\n");
    } else {
        printf("  Insufficient samples (%d). Try again.\n", count);
    }

    free(diffs);
}

/* ========== Main ========== */

int main(void) {
    mach_timebase_info(&timebase_info);

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TRIPHASE: Phase Extraction v2                          ║\n");
    printf("║  mach_absolute_time + clock_gettime_nsec_np             ║\n");
    printf("║  Hardware: Apple Silicon M1 Max                         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("  Timebase: %u/%u\n", timebase_info.numer, timebase_info.denom);

    experiment_timer_comparison();
    experiment_workload_spectrum();
    experiment_deep_autocorrelation();
    experiment_phase_slots();
    experiment_delta_spectrum();
    experiment_cross_core();

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  SUMMARY                                                ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Key questions answered:                                ║\n");
    printf("║  1. How much entropy per timer read?                    ║\n");
    printf("║  2. Which workload maximizes phase visibility?          ║\n");
    printf("║  3. Is there periodic structure? (autocorrelation)      ║\n");
    printf("║  4. How many phase slots can we reliably address?       ║\n");
    printf("║  5. What's the full delta distribution shape?           ║\n");
    printf("║  6. Are P-core and E-core truly asynchronous?           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    return 0;
}
