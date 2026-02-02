/*
 * test_libphit.c — Smoke test for libphit.h
 *
 * gcc -O0 -o test_libphit test_libphit.c -lm
 */

#define LIBPHIT_IMPLEMENTATION
#include "../src/libphit.h"

#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== libphit.h smoke test ===\n\n");

    /* Self-test */
    int st = phit_selftest();
    printf("Self-test:     %s\n", st ? "PASS" : "FAIL");

    /* Timer */
    uint64_t t = phit_now_ns();
    printf("Timer:         %llu ns\n", (unsigned long long)t);

    /* Sample */
    printf("Samples:       ");
    for (int i = 0; i < 5; i++) {
        printf("0x%08X ", phit_sample());
    }
    printf("\n");

    /* Compound */
    printf("Compound(2):   ");
    for (int i = 0; i < 5; i++) {
        printf("0x%08X ", phit_sample_compound(2));
    }
    printf("\n");

    /* Routing uniformity */
    int buckets[8] = {0};
    int N = 100000;
    for (int i = 0; i < N; i++) {
        buckets[phit_route(8)]++;
    }
    double expected = (double)N / 8.0;
    double chi2 = 0;
    printf("\nRouting (8 workers, %d tasks):\n", N);
    for (int i = 0; i < 8; i++) {
        double d = buckets[i] - expected;
        chi2 += d * d / expected;
        printf("  [%d] %6d", i, buckets[i]);
        int bar = (int)(buckets[i] * 30.0 / expected);
        if (bar > 40) bar = 40;
        printf(" ");
        for (int j = 0; j < bar; j++) printf("#");
        printf("\n");
    }
    printf("  Chi2=%.1f (uniform if <14.07)\n", chi2);

    /* PRNG */
    phit_prng_t rng;
    phit_prng_init(&rng);
    printf("\nPRNG output:\n");
    for (int i = 0; i < 5; i++) {
        printf("  0x%016llX  (%.6f)\n",
               (unsigned long long)phit_prng_u64(&rng),
               phit_prng_double(&rng));
    }

    /* Fill buffer */
    uint8_t buf[32];
    phit_prng_fill(&rng, buf, 32);
    printf("\nRandom bytes:  ");
    for (int i = 0; i < 32; i++) printf("%02X", buf[i]);
    printf("\n");

    /* Throughput */
    uint64_t t1 = phit_now_ns();
    int n = 50000;
    uint64_t x = 0;
    for (int i = 0; i < n; i++) {
        x ^= phit_prng_u64(&rng);
    }
    phit__sink = x;
    uint64_t t2 = phit_now_ns();
    double elapsed_s = (t2 - t1) / 1e9;
    double mbit_s = (n * 64.0) / elapsed_s / 1e6;
    printf("\nThroughput:    %.1f Mbit/s (%d values in %.1f ms)\n",
           mbit_s, n, elapsed_s * 1000);

    printf("\n=== Done ===\n");
    return st ? 0 : 1;
}
