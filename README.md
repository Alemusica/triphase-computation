# Triphase Computation

Computational model that treats phase relationships between asynchronous clocks
as an exploitable information resource, not noise to eliminate.

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
│   └── triphase_sim.py       # Core simulator (Clock, TriphaseSystem, VM)
├── experiments/
│   ├── phase_extract.c        # Hardware phase extraction (M1 Max / x86)
│   ├── practical_test.py      # Benchmark suite (6 tests)
│   └── beat_visualizer.py     # ASCII visualization of beat patterns
└── docs/
    └── THEORY.md              # Theoretical framework (CPE)
```

## Quick Start

```bash
# Run the benchmark suite
python3 experiments/practical_test.py

# Visualize beat patterns
python3 experiments/beat_visualizer.py

# Hardware test (compile on M1 Max)
gcc -O0 -o phase_extract experiments/phase_extract.c -lm
./phase_extract
```

## Three Paradigms

| # | Paradigm | Idea | Gain |
|---|----------|------|------|
| 1 | Phase-Gated | Compute only at sync points | Temporal filtering |
| 2 | Phase-Weighted | Phase as arithmetic coefficient | Same code → different results |
| 3 | Phase-Encoded | N values in 1 register via time-division | Nx memory density |

## Key Results (Simulation)

- **Memory density**: 8 slots/register = 3 extra bits = 8x values
- **Sync bonus**: ~2x throughput at beat alignment points
- **Hidden parallelism**: 1 sequential stream → 4 independent computations
- **Theoretical max on M1 Max**: ~7.1 extra bits/register (134 distinguishable phases)

## Hardware

Designed for Apple Silicon M1 Max with ARM timer (`cntvct_el0`, 24 MHz)
as external observer clock. Works on x86 via TSC as well.

## Author

Alessio Cazzaniga
