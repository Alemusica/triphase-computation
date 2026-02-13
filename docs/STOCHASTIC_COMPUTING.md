# Phit as Stochastic Computing Primitive

## Context

This document formalizes the connection between phit extraction and stochastic computing (Gaines, 1969). The insight: the phit is not a noise source for dithering. It is the **physical selector** of a stochastic ALU — the same role traditionally played by LFSRs, but asynchronous and free.

This document is canonical. It supersedes informal descriptions in chat transcripts.

---

## 1. Stochastic Computing in 60 Seconds

Stochastic computing (Gaines, 1969; Alaghi & Hayes, 2013) represents numbers as **probability streams**: the value 0.7 is a bitstream that is 1 with probability 0.7.

Operations become logic gates:
- **Multiplication:** AND gate. P(A AND B) = P(A) × P(B)
- **Scaled addition:** MUX with random selector. If selector S has P(S=0) = 0.5, output = 0.5×A + 0.5×B

The critical component is the **random selector**. Classical stochastic computing uses LFSRs (pseudo-random, deterministic, serially computed). The selector quality limits the computation quality.

**Phit replaces LFSR.** The hardware phase relationship between asynchronous clocks provides a physically asynchronous, non-computed random selector. This is the connection.

---

## 2. Why Phit ≠ Dithering

| Property | Dithering | Phit |
|---|---|---|
| Source | Generated (PRNG/LFSR) | Extracted (hardware phase) |
| Nature | Noise — destroys coherence | Information — deterministic but locally unpredictable |
| Cost | Computational (must be generated) | Zero (exists in silicon already) |
| Independence | Software pseudo-independence | Physical asynchrony (true independence) |
| Math | f(x) + noise | f(x, Φ) where Φ is a state variable |
| Role | Cosmetic (improves perception) | Structural (changes computation) |

The phit is governed by Weyl's Equidistribution Theorem: for incommensurable frequencies f₁, f₂, the phase Φ(t) = (f₁ - f₂)·t mod 1 visits every value in [0,1) with uniform density. This is deterministic — if you know f₁, f₂, and t exactly, you know Φ exactly. But it appears uniformly random to any observer that doesn't have all three values.

Dithering adds randomness. Phit **reveals** a hidden variable that was always there.

---

## 3. Algebra Compatibility

Not all algebraic structures can use phase. The following table maps which ones can:

| Algebra | Operations | Phase-compatible? | How |
|---|---|---|---|
| ℝ standard | a×b = c (deterministic) | No | Result is fixed |
| **Stochastic** | E[a×b] = c (mean deterministic) | **Yes** | Phase is the MUX selector |
| Modular | a×b mod p (deterministic per p) | Potentially | Research direction |
| **Fuzzy** | a×b ∈ [c-ε, c+ε] | **Yes** | Phase defines ε |
| **Tropical** | max(a,b) and a+b as ops | **Yes** | Phase as min/max selector |

The key insight: **no new algebra is required.** Stochastic computing has existed since 1969. Phase is simply a better entropy source for it — physically asynchronous rather than computationally pseudo-random.

---

## 4. Three Levels of Computational Gain

### Level 1: Software Extraction (Today — libphit.h)

- Timer read every ~100 ns
- ~4 phits per compound read
- Phase rotation per buffer
- Throughput: 24.6 Mphit/s

**This level does not multiply FLOPs.** It extracts entropy for routing, PRNG, and crypto. The previous THEORY.md and patent claims cover this level completely.

### Level 2: GPU Integration (Near-Term)

- GPU timestamp as 4th clock domain
- Each GPU threadgroup has different phase → natural decorrelation
- Phase-weighted kernels: computation varies with clock domain relationship
- 32 FDN lines × 48000 samples/s = 1.5M phase-weighted ops/s

At this level, phase drives **scheduling and coefficient variation**. Different from Level 1 because phase is an input to computation, not just an entropy source.

### Level 3: RDPHI Hardware (ISA Extension)

- 0.3 ns per phase read ≈ 1 phase per FLOP at 3 GHz
- Every FLOP can be phase-weighted
- Every register can be phase-encoded (K values via time-division)
- Every branch can be phase-gated

At this level, the extended state model applies in full:

```
Σ(t) = (S(t), Φ(t))

H_total = H(S) + H(Φ|S)

With K distinguishable phase levels:
|state space| = 2^N × K  (where N = register bits)
```

**This IS a computational multiplier.** The register contains more information than its bit width. Phase-encoded state (Paradigm 3) stores K values in 1 register via phase windows. The silicon is the same, but the information capacity of each register increases by factor K.

---

## 5. Stochastic ALU with Phase Selector

Classical stochastic ALU:

```
Input A ──────────┐
                   ├── MUX ── Output
Input B ──────────┘
                   │
LFSR ─── selector ─┘
```

Phase-stochastic ALU:

```
Input A ──────────┐
                   ├── MUX ── Output
Input B ──────────┘
                   │
RDPHI ── selector ─┘
```

The LFSR is replaced by a hardware phase comparator. The difference:
- LFSR: deterministic, periodic (period = 2^n - 1), must be computed
- RDPHI: deterministic but aperiodic (Weyl), physically parallel, zero compute cost

On FPGA with genuinely asynchronous clock domains:
- Each stochastic ALU has its own phase source
- Sources are physically independent (not divided from a common PLL)
- With K clock domains: K(K-1)/2 independent phase pairs
- Information scales **combinatorially** with clock domains, not linearly

---

## 6. FPGA Validation Path

To demonstrate real (not simulated) phase computation on FPGA:

1. **3 PLLs at incommensurable frequencies** → genuine clock domains (not divided from common reference)
2. **Phase comparators** → 1-cycle RDPHI registers (flip-flop sampling clock A on edge of clock B)
3. **Stochastic ALU** with phase-driven MUX selector
4. **Benchmark:** phase-stochastic vs LFSR-based classical stochastic computing
5. **Measure:** accuracy, throughput, power, gate count

Expected result: equivalent accuracy to LFSR-based stochastic computing, with zero additional compute for the random selector, and natural multi-domain parallelism.

---

## References

- Gaines, B.R. (1969). "Stochastic Computing Systems." Advances in Information Systems Science, 2, 37-172.
- Alaghi, A. & Hayes, J.P. (2013). "Survey of Stochastic Computing." ACM TODAES, 18(2).
- Weyl, H. (1916). "Über die Gleichverteilung von Zahlen mod. Eins." Math. Annalen, 77, 313-352.
- Three-Distance Theorem (Steinhaus, 1957; Sós, 1958).

---

*This document is part of the Triphase Computation project.*
*Author: Alessio Cazzaniga*
*License: BSL 1.1 (see LICENSE)*
