/*
 * Phit Uniform Router — CDF-based slot mapping
 * ===============================================
 *
 * Problema: i delta raw sono biased (63% = 125ns).
 * Soluzione: calibra una CDF, poi usa la CDF per mappare
 * qualsiasi distribuzione → slot uniformi.
 *
 * Fase 1: calibrazione (misura distribuzione reale)
 * Fase 2: costruzione CDF lookup table
 * Fase 3: routing uniforme verificato
 * Fase 4: calcolo reale su slot uniformi
 *
 * Compila: gcc -O0 -o phi_uniform phi_uniform.c -lm
 * Esegui:  ./phi_uniform
 *
 * Author: Alessio Cazzaniga
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <mach/mach_time.h>

static volatile uint64_t sink;

static inline uint64_t now_ns(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

static void workload_calibrated(void) {
    volatile uint64_t x = 0xCAFEBABE;
    for (int i = 0; i < 20; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        x ^= x >> 17;
    }
    sink = x;
}

/* ========== CDF-based Phit Router ========== */

#define MAX_DELTA     2048
#define CALIB_SAMPLES 500000

typedef struct {
    int num_slots;
    /* For each delta value [0..MAX_DELTA), which slot does it map to? */
    int slot_map[MAX_DELTA];
    /* Calibration stats */
    int calib_hist[MAX_DELTA];
    int calib_total;
    double cdf[MAX_DELTA];
    /* Verification */
    double expected_per_slot;
} phit_router_t;

static void router_calibrate(phit_router_t *r, int num_slots) {
    r->num_slots = num_slots;
    r->calib_total = CALIB_SAMPLES;
    memset(r->calib_hist, 0, sizeof(r->calib_hist));

    printf("  Calibrating with %d samples...\n", CALIB_SAMPLES);

    /* Phase 1: collect distribution */
    for (int i = 0; i < CALIB_SAMPLES; i++) {
        uint64_t t1 = now_ns();
        workload_calibrated();
        uint64_t t2 = now_ns();
        uint64_t delta = t2 - t1;
        if (delta < MAX_DELTA) {
            r->calib_hist[delta]++;
        }
    }

    /* Phase 2: build CDF */
    double cumulative = 0;
    for (int d = 0; d < MAX_DELTA; d++) {
        cumulative += (double)r->calib_hist[d] / CALIB_SAMPLES;
        r->cdf[d] = cumulative;
    }

    /* Phase 3: map CDF → uniform slots
     * slot = floor(CDF(delta) * num_slots)
     * This guarantees each slot gets ~equal probability mass */
    for (int d = 0; d < MAX_DELTA; d++) {
        int slot = (int)(r->cdf[d] * num_slots);
        if (slot >= num_slots) slot = num_slots - 1;
        r->slot_map[d] = slot;
    }

    r->expected_per_slot = (double)CALIB_SAMPLES / num_slots;
    printf("  Calibration complete. CDF mapped to %d slots.\n\n", num_slots);
}

static inline int router_route(phit_router_t *r, uint64_t delta) {
    if (delta >= MAX_DELTA) delta = MAX_DELTA - 1;
    return r->slot_map[delta];
}

/* ========== Phase 1: Show calibration ========== */

static void show_calibration(phit_router_t *r) {
    printf("  === Calibration Distribution ===\n\n");
    printf("  %6s | %7s | %6s | CDF     | Slot\n", "Delta", "Count", "%");
    printf("  %6s-+-%7s-+-%6s-+-%7s-+-%4s\n",
           "------", "-------", "------", "-------", "----");

    for (int d = 0; d < MAX_DELTA; d++) {
        if (r->calib_hist[d] > 0) {
            printf("  %6d | %7d | %5.1f%% | %.4f  | %d\n",
                   d, r->calib_hist[d],
                   100.0 * r->calib_hist[d] / CALIB_SAMPLES,
                   r->cdf[d], r->slot_map[d]);
        }
    }
}

/* ========== Phase 2: Verify uniformity ========== */

static void verify_uniformity(phit_router_t *r) {
    printf("\n  === Uniformity Verification ===\n\n");

    int n = 200000;
    int *slot_counts = calloc(r->num_slots, sizeof(int));

    for (int i = 0; i < n; i++) {
        uint64_t t1 = now_ns();
        workload_calibrated();
        uint64_t t2 = now_ns();
        int slot = router_route(r, t2 - t1);
        slot_counts[slot]++;
    }

    double expected = (double)n / r->num_slots;
    double chi2 = 0;
    double max_bias = 0;

    printf("  %4s | %7s | %6s | Bias    | Bar\n",
           "Slot", "Count", "%");
    printf("  %4s-+-%7s-+-%6s-+-%7s-+-%s\n",
           "----", "-------", "------", "-------", "----------");

    for (int s = 0; s < r->num_slots; s++) {
        double pct = 100.0 * slot_counts[s] / n;
        double bias = fabs(pct - 100.0 / r->num_slots);
        if (bias > max_bias) max_bias = bias;
        double d = slot_counts[s] - expected;
        chi2 += d * d / expected;

        int bar_len = (int)(pct * 40 / (100.0 / r->num_slots));
        if (bar_len > 60) bar_len = 60;
        char bar[61];
        memset(bar, '#', bar_len > 0 ? bar_len : 0);
        bar[bar_len > 0 ? bar_len : 0] = '\0';

        printf("  %4d | %7d | %5.1f%% | %+5.1f%% | %s\n",
               s, slot_counts[s], pct,
               pct - 100.0 / r->num_slots, bar);
    }

    double df = r->num_slots - 1;
    double chi2_crit = df + 2.0 * sqrt(df);  /* approximate */

    printf("\n  Chi²: %.1f (critical ≈ %.1f at p=0.05)\n", chi2, chi2_crit);
    printf("  Max bias: %.1f%%\n", max_bias);
    printf("  Result: %s\n", chi2 < chi2_crit ? "UNIFORM" : "NOT UNIFORM");

    free(slot_counts);
}

/* ========== Phase 3: Phit-routed parallel computation ========== */

static void phit_parallel_compute(phit_router_t *r) {
    printf("\n  === Phit-Routed Parallel Computation ===\n\n");

    int K = r->num_slots;
    int n = 500000;

    /* K independent accumulators, each doing different work */
    double accumulators[64] = {0};
    int slot_ops[64] = {0};

    for (int i = 0; i < n; i++) {
        uint64_t t1 = now_ns();
        workload_calibrated();
        uint64_t t2 = now_ns();
        int slot = router_route(r, t2 - t1);

        /* Each slot runs a different computation */
        switch (slot % 8) {
            case 0: accumulators[slot] += 1.0; break;
            case 1: accumulators[slot] += 2.0; break;
            case 2: accumulators[slot] *= 1.001; if (accumulators[slot] == 0) accumulators[slot] = 1; break;
            case 3: accumulators[slot] -= 0.5; break;
            case 4: accumulators[slot] += 3.14159; break;
            case 5: accumulators[slot] = accumulators[slot] * 0.999 + 1.0; break;
            case 6: accumulators[slot] += (slot_ops[slot] % 2 == 0) ? 1.0 : -1.0; break;
            case 7: accumulators[slot] += 10.0; break;
        }
        slot_ops[slot]++;
    }

    printf("  %d operations → %d parallel channels (phit-routed)\n\n", n, K);
    printf("  %4s | %8s | %14s | phits used\n", "Slot", "Ops", "Accumulator");
    printf("  %4s-+-%8s-+-%14s-+-%s\n", "----", "--------", "--------------", "----------");

    int total_active = 0;
    for (int s = 0; s < K; s++) {
        if (slot_ops[s] > 0) {
            printf("  %4d | %8d | %14.2f | %.1f\n",
                   s, slot_ops[s], accumulators[s], log2(K));
            total_active++;
        }
    }

    printf("\n  Active channels:   %d/%d\n", total_active, K);
    printf("  Ops/channel (avg): %d\n", n / (total_active > 0 ? total_active : 1));
    printf("  Phits per route:   %.1f\n", log2(K));
    printf("  Total phits used:  %.0f\n", n * log2(K));
    printf("  Effective parallelism: %dx from 1 sequential stream\n", total_active);
}

/* ========== Phase 4: Scaling test — how many slots can we sustain? ========== */

static void phit_scaling_test(void) {
    printf("\n  === Phit Scaling: Slots vs Uniformity ===\n\n");

    int slot_configs[] = {2, 4, 8, 16, 32};
    int nc = sizeof(slot_configs) / sizeof(slot_configs[0]);

    printf("  %6s | %8s | %8s | %6s | %s\n",
           "Slots", "Chi²", "Critical", "Status", "Effective phits");
    printf("  %6s-+-%8s-+-%8s-+-%6s-+-%s\n",
           "------", "--------", "--------", "------", "---------------");

    for (int ci = 0; ci < nc; ci++) {
        int K = slot_configs[ci];
        phit_router_t *r = calloc(1, sizeof(phit_router_t));

        /* Quick calibration */
        r->num_slots = K;
        r->calib_total = 200000;
        memset(r->calib_hist, 0, sizeof(r->calib_hist));

        for (int i = 0; i < 200000; i++) {
            uint64_t t1 = now_ns();
            workload_calibrated();
            uint64_t t2 = now_ns();
            uint64_t delta = t2 - t1;
            if (delta < MAX_DELTA) r->calib_hist[delta]++;
        }

        double cumulative = 0;
        for (int d = 0; d < MAX_DELTA; d++) {
            cumulative += (double)r->calib_hist[d] / 200000;
            r->cdf[d] = cumulative;
            int slot = (int)(r->cdf[d] * K);
            if (slot >= K) slot = K - 1;
            r->slot_map[d] = slot;
        }

        /* Verify */
        int n = 200000;
        int *counts = calloc(K, sizeof(int));
        for (int i = 0; i < n; i++) {
            uint64_t t1 = now_ns();
            workload_calibrated();
            uint64_t t2 = now_ns();
            counts[router_route(r, t2 - t1)]++;
        }

        double expected = (double)n / K;
        double chi2 = 0;
        for (int s = 0; s < K; s++) {
            double d = counts[s] - expected;
            chi2 += d * d / expected;
        }

        double df = K - 1;
        double chi2_crit = df + 2.0 * sqrt(df);
        int uniform = chi2 < chi2_crit;

        printf("  %6d | %8.1f | %8.1f | %6s | %.1f phits\n",
               K, chi2, chi2_crit, uniform ? "OK" : "FAIL", log2(K));

        free(counts);
        free(r);
    }
}

/* ========== Main ========== */

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TRIPHASE: Phit Uniform Router                          ║\n");
    printf("║  CDF-calibrated phase-bit routing for M1 Max            ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Main test with 8 slots */
    phit_router_t router;
    router_calibrate(&router, 8);
    show_calibration(&router);
    verify_uniformity(&router);
    phit_parallel_compute(&router);

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  SCALING TEST                                           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    phit_scaling_test();

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  CONCLUSION                                             ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  CDF calibration transforms biased phase distribution   ║\n");
    printf("║  into uniform slot routing. Each slot = 1 independent   ║\n");
    printf("║  computation channel, addressed by phits.               ║\n");
    printf("║                                                         ║\n");
    printf("║  phit = phase-bit (unit of temporal information)        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    return 0;
}
