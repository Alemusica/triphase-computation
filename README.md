# Triphase Computation

**Your CPU has multiple asynchronous clocks. Their phase relationship is information. This library extracts it.**

Every modern processor contains independent clock domains — performance cores, efficiency cores, system timers — running at different frequencies. The phase relationship between them changes continuously and deterministically, but appears random to an observer who doesn't know the exact frequencies and timing.

Current engineering treats this as noise. We treat it as a computational resource.

## The phit

A **phit** (phase-bit) is the unit of information extracted from the phase relationship between two asynchronous clock domains. On Apple M1 Max:

| Metric | Value |
|---|---|
| Phits per single read | 1.96 |
| Phits per compound read (N=2) | 4.06 |
| Extraction throughput | 24.6 Mphit/s |

The uniformity of timer LSBs is provable: Chi-squared = 0.39 across 4 bins (critical value 7.81 at p=0.05). This isn't statistical luck — it's a consequence of Weyl's Equidistribution Theorem applied to incommensurable clock frequencies.

## What it does

### PRNG from phase entropy

Random number generator seeded by clock phase relationships. Passes four NIST SP 800-22-inspired statistical tests on 10M samples:

```
Monobit:           PASS (Z-score < 3.29)
Runs:              PASS (Z-score < 3.29)
Byte distribution: PASS (Chi² = 221.7, critical = 310)
Per-bit entropy:   PASS (64.0/64.0 bits)
Throughput:        181 Mbit/s
```

### Lock-free task routing

Distribute tasks to workers using phase instead of shared counters. No mutex, no atomics, no shared memory.

```
Workers:      8
Tasks:        100,000
Chi²:         7.8 (uniform threshold: 14.07)
Max imbalance: 2.5%
Throughput:    28 Mroute/s
Shared state:  none
```

### Phase-gated encryption

Encryption where the key is not stored — it's the temporal relationship between clock frequencies at a specific instant. 1 microsecond timing error = completely different output.

## Quick start

```bash
# Build and run tests
gcc -O0 -o test_libphit tests/test_libphit.c -lm && ./test_libphit

# Run the PRNG benchmark
gcc -O0 -o phit_prng src/phit_prng.c -lm && ./phit_prng

# Run the lock-free scheduler
gcc -O0 -o phit_scheduler src/phit_scheduler.c -lm -lpthread && ./phit_scheduler

# Run the encryption demo
gcc -O0 -o phit_crypto src/phit_crypto.c -lm && ./phit_crypto
```

**Important:** compile with `-O0`. Optimization can eliminate the calibrated workload that makes phase extraction work.

## Using libphit.h

Header-only. One file. No dependencies beyond libc.

```c
#define LIBPHIT_IMPLEMENTATION
#include "libphit.h"

// Extract phase information
uint32_t sample = phit_sample_compound(2);  // ~4 phits

// Route a task (lock-free, zero shared state)
int worker = phit_route(num_workers);

// Generate random numbers
phit_prng_t rng;
phit_prng_init(&rng);
uint64_t r = phit_prng_u64(&rng);

// Self-test validates extraction on your hardware
assert(phit_selftest());
```

Platforms: macOS (ARM64, x86), Windows (MSVC, MinGW), Linux (ARM64, x86), FreeBSD.

## Structure

```
src/
  libphit.h           Header-only library (366 lines)
  phit_prng.c          PRNG benchmark (NIST-inspired tests)
  phit_crypto.c        Phase-gated encryption demo
  phit_scheduler.c     Lock-free task routing demo
tests/
  test_libphit.c       Smoke test + throughput measurement
experiments/
  phase_extract.c      Phase extraction v1 (cntvct_el0 direct)
  phase_extract_v2.c   Phase extraction v2 (mach + clock_gettime)
  phi_exploit.c        Phit maximization experiments
  phi_uniform.c        CDF-based routing (non-stationarity finding)
  phi_adaptive.c       Adaptive compound-key routing
  practical_test.py    Python benchmark suite
  beat_visualizer.py   ASCII beat pattern visualization
```

## Theory

The extended processor state at time t:

```
Sigma(t) = (S(t), Phi(t))

S(t)   = spatial state (registers, memory)
Phi(t) = phase vector (f_1*t mod 1, f_2*t mod 1, ..., f_K*t mod 1)
```

Three asynchronous clocks form a **triad**: two generate a beat signal, a third observes and computes based on the phase relationship. Neither clock can observe its own phase — only the third can.

Three computational paradigms:

| Paradigm | Mechanism | Application |
|---|---|---|
| Phase-Gated | Execute only when clocks align | Temporal filtering, access windows |
| Phase-Weighted | Phase as arithmetic coefficient | Natural decorrelation, dithering |
| Phase-Encoded | N values in 1 register via time-division | Memory density multiplication |

## Proposed ISA extension

Three instructions for direct hardware phase readout (RISC-V custom extension):

```
RDPHI rd, rs1, rs2    Read phase between clock domains rs1 and rs2
PHIGATE rd, rs1        Gate execution on phase condition
PHIWEIGHT rd, rs1      Return phase as fixed-point arithmetic weight
```

Hardware cost: ~200 logic gates (flip-flop phase comparator). Projected improvement: 100-300x throughput over software extraction.

## Prior art and differentiation

| Approach | What it does | How this differs |
|---|---|---|
| Jitterentropy | Collects CPU timing noise (~1 bit/sample) | Formalizes phase *relationship*, extracts ~4 phits/sample, uses for routing/crypto |
| RDRAND/RDSEED | Dedicated silicon RNG | Uses existing clocks, no hardware modification |
| Clock Machines (US10498528B2) | Single clock, discrete time states | Multiple async clocks, continuous phase |
| RISC-V entropy source | Returns entropy only | Returns phase for general-purpose computation |

## License

**Business Source License 1.1** (BSL). You may freely use this software for non-production purposes: personal projects, research, evaluation, testing, education. Production and commercial use require a separate license.

On February 13, 2030 (the Change Date), the license converts to Apache 2.0.

Contact: alessio.cazzaniga87@gmail.com

## Patent

The methods implemented in this software are the subject of pending patent applications filed with the Italian Patent and Trademark Office (UIBM), February 2026. Use of these methods — whether through this software or independent implementation — may require a patent license in applicable jurisdictions.

## Author

Alessio Cazzaniga
