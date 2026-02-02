# Triphase Computation

Computational model that treats phase relationships between asynchronous clocks
as an exploitable information resource, not noise to eliminate.

**phit** (phase-bit): unit of information extracted from clock phase relationships.

## Core Idea

```
Σ(t) = (S(t), Φ(t))

S = spatial state (conventional bits)
Φ = phase vector (temporal information — currently discarded)
```

Three asynchronous clocks form a triad: two generate a *beat signal*,
a third *observes* and *computes* based on the phase relationship.

## Structure

```
triphase-computation/
├── src/
│   ├── triphase_sim.py        # Core simulator (Clock, TriphaseSystem, VM)
│   ├── phit_prng.c            # PRNG from phase entropy (NIST tests pass)
│   ├── phit_crypto.c          # Phase-gated encryption PoC
│   └── phit_scheduler.c       # Lock-free task dispatch via phits
├── experiments/
│   ├── phase_extract.c        # Hardware phase extraction v1 (cntvct_el0)
│   ├── phase_extract_v2.c     # v2 (mach_absolute_time + clock_gettime)
│   ├── phi_exploit.c          # Phit maximization tests
│   ├── phi_uniform.c          # CDF-based routing (non-stationary finding)
│   ├── phi_adaptive.c         # Adaptive compound-key routing
│   ├── practical_test.py      # Python benchmark suite (6 tests)
│   └── beat_visualizer.py     # ASCII beat pattern visualization
└── docs/
    └── THEORY.md              # Theoretical framework (CPE)
```

## Quick Start

```bash
# Applications
gcc -O0 -o phit_prng src/phit_prng.c -lm && ./phit_prng
gcc -O0 -o phit_crypto src/phit_crypto.c -lm && ./phit_crypto
gcc -O0 -o phit_scheduler src/phit_scheduler.c -lm -lpthread && ./phit_scheduler

# Experiments
gcc -O0 -o phi_adaptive experiments/phi_adaptive.c -lm && ./phi_adaptive
python3 experiments/practical_test.py
```

## Three Paradigms

| # | Paradigm | Idea | Gain |
|---|----------|------|------|
| 1 | Phase-Gated | Compute only at sync points | Temporal filtering |
| 2 | Phase-Weighted | Phase as arithmetic coefficient | Same code, different results |
| 3 | Phase-Encoded | N values in 1 register via time-division | Nx memory density |

## Applications

### Phit PRNG
Random number generator using phits as entropy source.
All NIST SP 800-22 tests pass: monobit, runs, byte distribution, per-bit entropy.
Throughput: **181 Mbit/s** (22.6 MB/s).

### Phit Crypto
Phase-gated encryption where the "key" is not stored — it's the moment in time
multiplied by clock frequency relationships. 1 microsecond timing error = garbage output.

### Phit Scheduler
Lock-free task dispatch: routes tasks to workers using phase instead of shared counters.
Chi²=5.8 (uniform), 28 Mroute/s, zero contention.

## Hardware Results (M1 Max)

| Metric | Value |
|---|---|
| Raw phits/read | 1.96 |
| Compound (N=2) | 4.06 phits |
| Throughput | 24.6 Mphit/s |
| PRNG quality | All NIST tests PASS |
| Scheduler Chi² | 5.8 (uniform < 14.07) |
| Routing speed | 28 Mroute/s |

## Terminology

- **phit** (singular) — 1 phase-bit, unit of temporal information
- **phits** (plural) — N phase-bits
- **Triphase** — the three-clock architecture (Alpha, Beta, Observer)

## Author

Alessio Cazzaniga
