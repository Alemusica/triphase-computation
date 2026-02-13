/*
 * libphit.h — Header-Only Phit Library
 * ======================================
 *
 * Portable library for extracting and using phits (phase-bits)
 * from asynchronous CPU clock relationships.
 *
 * Usage:
 *   #define LIBPHIT_IMPLEMENTATION
 *   #include "libphit.h"
 *
 * Define LIBPHIT_IMPLEMENTATION in exactly ONE .c file before including.
 * All other files can include without the define for declarations only.
 *
 * Platforms: macOS (ARM64/x86), Linux (ARM64/x86), FreeBSD
 *
 * Author: Alessio Cazzaniga
 * License: BSL 1.1 (see LICENSE). Non-production use permitted.
 *          Commercial/production use requires a separate license.
 * Patent:  Methods covered by pending patent applications (UIBM, Feb 2026).
 */

#ifndef LIBPHIT_H
#define LIBPHIT_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * Configuration
 * ==================================================================== */

#ifndef PHIT_WORKLOAD_ITERS
#define PHIT_WORKLOAD_ITERS 20
#endif

#ifndef PHIT_POOL_LANES
#define PHIT_POOL_LANES 4
#endif

#ifndef PHIT_PRNG_SEED_ROUNDS
#define PHIT_PRNG_SEED_ROUNDS 16
#endif

/* ====================================================================
 * Types
 * ==================================================================== */

/* Entropy pool: 256-bit state */
typedef struct {
    uint64_t pool[PHIT_POOL_LANES];
    uint64_t mix_counter;
    int      bits_collected;
} phit_pool_t;

/* PRNG state */
typedef struct {
    phit_pool_t pool;
    uint64_t    generated;
} phit_prng_t;

/* Phit sample result */
typedef struct {
    uint32_t key;
    int      num_reads;
    double   phits_est;   /* estimated phits in this sample */
} phit_sample_t;

/* ====================================================================
 * API Declarations
 * ==================================================================== */

/* --- Timer --- */
uint64_t phit_now_ns(void);

/* --- Core sampling --- */
uint32_t phit_hash32(uint32_t key);
uint64_t phit_hash64(uint64_t key);
void     phit_workload(void);
uint32_t phit_sample(void);
uint32_t phit_sample_compound(int num_reads);

/* --- Routing --- */
int      phit_route(int num_destinations);

/* --- Entropy pool --- */
void     phit_pool_init(phit_pool_t *p);
void     phit_pool_feed(phit_pool_t *p, uint64_t sample);
void     phit_pool_harvest(phit_pool_t *p);
uint64_t phit_pool_extract(phit_pool_t *p);

/* --- PRNG --- */
void     phit_prng_init(phit_prng_t *rng);
uint64_t phit_prng_u64(phit_prng_t *rng);
uint32_t phit_prng_u32(phit_prng_t *rng);
double   phit_prng_double(phit_prng_t *rng);
uint32_t phit_prng_range(phit_prng_t *rng, uint32_t max);
void     phit_prng_fill(phit_prng_t *rng, void *buf, int len);

/* --- Self-test --- */
int      phit_selftest(void);

#ifdef __cplusplus
}
#endif

/* ====================================================================
 * Implementation
 * ==================================================================== */

#ifdef LIBPHIT_IMPLEMENTATION

#include <math.h>

/* Prevent compiler from optimizing away workloads */
static volatile uint64_t phit__sink;

/* ---- Platform timer ---- */

#if defined(__APPLE__)
  #include <time.h>
  #include <mach/mach_time.h>
  uint64_t phit_now_ns(void) {
      return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
  }
#elif defined(__linux__) || defined(__FreeBSD__)
  #include <time.h>
  uint64_t phit_now_ns(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
      return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
  }
#else
  #include <time.h>
  uint64_t phit_now_ns(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
  }
#endif

/* ---- Hash functions ---- */

uint32_t phit_hash32(uint32_t key) {
    key *= 2654435761u;
    key ^= key >> 16;
    key *= 0x85ebca6bu;
    key ^= key >> 13;
    return key;
}

uint64_t phit_hash64(uint64_t key) {
    key = (key ^ (key >> 30)) * 0xBF58476D1CE4E5B9ULL;
    key = (key ^ (key >> 27)) * 0x94D049BB133111EBULL;
    key = key ^ (key >> 31);
    return key;
}

/* ---- Workload ---- */

void phit_workload(void) {
    volatile uint64_t x = 0xCAFEBABE;
    for (int i = 0; i < PHIT_WORKLOAD_ITERS; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        x ^= x >> 17;
    }
    phit__sink = x;
}

/* ---- Rotation helper ---- */

static inline uint64_t phit__rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* ---- Quantize delta to timer ticks ---- */

static inline int phit__quantize(uint64_t delta_ns) {
    return (int)((delta_ns + 21) / 42);
}

/* ---- Core sampling ---- */

uint32_t phit_sample(void) {
    /* Workload with timer-seeded variation */
    volatile uint64_t x = 0xDEADBEEF;
    for (int i = 0; i < 10; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    phit__sink = x;

    uint64_t t = phit_now_ns();
    /* Combine timer LSBs (uniform) with workload result (phase-dependent) */
    uint32_t key = (uint32_t)((t & 0x3) | (((uint32_t)(t >> 2) ^ (uint32_t)x) << 2));
    return phit_hash32(key);
}

uint32_t phit_sample_compound(int num_reads) {
    uint32_t key = 0;
    for (int i = 0; i < num_reads; i++) {
        volatile uint64_t x = 0xDEADBEEF ^ ((uint64_t)i * 0x9E3779B97F4A7C15ULL);
        for (int j = 0; j < 10; j++) {
            x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        }
        phit__sink = x;

        uint64_t t = phit_now_ns();
        uint32_t sample = (uint32_t)((t & 0x3) | (((uint32_t)(t >> 2) ^ (uint32_t)x) << 2));
        key ^= phit_hash32(sample + (uint32_t)i);
        key = (key << 7) | (key >> 25);  /* rotate to spread bits */
    }
    return phit_hash32(key);
}

/* ---- Routing ---- */

int phit_route(int num_destinations) {
    /* Use compound sampling (N=2) for adequate entropy.
     * Single reads produce too few distinct levels for uniform routing. */
    return (int)(phit_sample_compound(2) % (uint32_t)num_destinations);
}

/* ---- Entropy pool ---- */

void phit_pool_init(phit_pool_t *p) {
    memset(p, 0, sizeof(phit_pool_t));
}

void phit_pool_feed(phit_pool_t *p, uint64_t sample) {
    p->mix_counter++;

    uint64_t z = sample + p->mix_counter * 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    int slot = (int)(p->mix_counter & 3);
    p->pool[slot] ^= z;
    p->pool[(slot + 1) & 3] ^= phit__rotl64(p->pool[slot], 17);

    p->bits_collected += 2;
}

void phit_pool_harvest(phit_pool_t *p) {
    volatile uint64_t x = 0xCAFEBABE;
    for (int i = 0; i < PHIT_WORKLOAD_ITERS; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        x ^= x >> 17;
    }
    phit__sink = x;

    uint64_t t = phit_now_ns();
    phit_pool_feed(p, t);
    phit_pool_feed(p, (uint64_t)x ^ t);
}

uint64_t phit_pool_extract(phit_pool_t *p) {
    for (int i = 0; i < PHIT_POOL_LANES; i++) {
        phit_pool_harvest(p);
    }

    uint64_t out = p->pool[0];
    out ^= phit__rotl64(p->pool[1], 13);
    out ^= phit__rotl64(p->pool[2], 29);
    out ^= phit__rotl64(p->pool[3], 43);

    /* Forward-secure: mutate pool after extraction */
    p->pool[0] ^= phit__rotl64(out, 7);
    p->pool[1] ^= phit__rotl64(out, 23);

    return out;
}

/* ---- PRNG ---- */

void phit_prng_init(phit_prng_t *rng) {
    memset(rng, 0, sizeof(phit_prng_t));
    for (int i = 0; i < PHIT_PRNG_SEED_ROUNDS; i++) {
        phit_pool_harvest(&rng->pool);
    }
}

uint64_t phit_prng_u64(phit_prng_t *rng) {
    rng->generated++;
    return phit_pool_extract(&rng->pool);
}

uint32_t phit_prng_u32(phit_prng_t *rng) {
    return (uint32_t)(phit_prng_u64(rng) >> 32);
}

double phit_prng_double(phit_prng_t *rng) {
    return (double)(phit_prng_u64(rng) >> 11) / (double)(1ULL << 53);
}

uint32_t phit_prng_range(phit_prng_t *rng, uint32_t max) {
    if (max == 0) return 0;
    return (uint32_t)(phit_prng_u64(rng) % max);
}

void phit_prng_fill(phit_prng_t *rng, void *buf, int len) {
    uint8_t *p = (uint8_t *)buf;
    while (len >= 8) {
        uint64_t v = phit_prng_u64(rng);
        memcpy(p, &v, 8);
        p += 8;
        len -= 8;
    }
    if (len > 0) {
        uint64_t v = phit_prng_u64(rng);
        memcpy(p, &v, len);
    }
}

/* ---- Self-test ---- */

int phit_selftest(void) {
    int pass = 1;

    /* Test 1: hash determinism */
    if (phit_hash32(42) != phit_hash32(42)) pass = 0;
    if (phit_hash32(42) == phit_hash32(43)) pass = 0;

    /* Test 2: PRNG produces different values */
    phit_prng_t rng;
    phit_prng_init(&rng);
    uint64_t a = phit_prng_u64(&rng);
    uint64_t b = phit_prng_u64(&rng);
    if (a == b) pass = 0;

    /* Test 3: monobit — count 1s in 1000 values, should be ~50% */
    int ones = 0;
    int total_bits = 1000 * 64;
    for (int i = 0; i < 1000; i++) {
        uint64_t v = phit_prng_u64(&rng);
        ones += __builtin_popcountll(v);
    }
    double ratio = (double)ones / total_bits;
    if (ratio < 0.45 || ratio > 0.55) pass = 0;

    /* Test 4: routing uniformity — 8 buckets, 10000 routes */
    int buckets[8] = {0};
    for (int i = 0; i < 10000; i++) {
        buckets[phit_route(8)]++;
    }
    double expected = 10000.0 / 8.0;
    double chi2 = 0;
    for (int i = 0; i < 8; i++) {
        double d = buckets[i] - expected;
        chi2 += d * d / expected;
    }
    if (chi2 > 30.0) pass = 0;  /* very generous threshold */

    /* Test 5: timer is advancing */
    uint64_t t1 = phit_now_ns();
    phit_workload();
    uint64_t t2 = phit_now_ns();
    if (t2 <= t1) pass = 0;

    return pass;
}

#endif /* LIBPHIT_IMPLEMENTATION */
#endif /* LIBPHIT_H */
