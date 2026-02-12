# Triphase Computation — Roadmap

*Last updated: 2026-02-12*

---

## Phase 0: Patent Filing (NOW)

### UIBM Italian Patent
- [x] Provisional US specification prepared
- [x] UIBM filing structure created (docs/uibm/)
- [x] Claims reduced to 10 (3 independent + 7 dependent)
- [x] Prior art updated (US10498528B2, RISC-V entropy, jitterentropy)
- [x] PDFs generated
- [ ] CIE digital signature on PDFs
- [ ] Upload to UIBM portal (servizionline.uibm.gov.it)
- [ ] F24 payment (EUR 50, codice tributo 6936)
- [ ] Receive priority date confirmation

### Claim Structure (Final)
| # | Type | Claim | Strength |
|---|------|-------|----------|
| 1 | Independent | Phit extraction method | MEDIUM — needs rewrite to differentiate from jitterentropy |
| 2 | Independent | Lock-free task routing via phase | STRONG — no prior art found |
| 3 | Independent | RDPHI processor instruction | MEDIUM — no implementation yet |
| 4 | Dependent→1 | Compound sampling N=2 optimal | OK |
| 5 | Dependent→1 | Timer LSB uniformity validation | STRONG |
| 6 | Dependent→1 | PRNG with entropy pool + forward security | OK |
| 7 | Dependent→1 | Phase-gated encryption | WEAK — needs reframe as access control |
| 8 | Dependent→7 | Phase-locked access windows | WEAK — inherits crypto weakness |
| 9 | Dependent→3 | PHIGATE/PHIWEIGHT companion instructions | OK |
| 10 | Dependent→1 | Portable software library | OK |

### Prior Art to Cite (MUST add before non-provisional)
- US10498528B2 "Clock Machines" (Kwok, 2019) — single clock discrete vs multi-clock async
- RISC-V entropy source interface (Neumann et al., 2022) — entropy-only vs computation
- Jitterentropy (Muller) — ~1 bit/sample vs 4.06 phit/sample
- GALS architecture — synchronization vs computation
- Time-to-Digital Converters — measurement vs ISA exposure
- Physical Unclonable Functions — inter-device vs intra-device temporal
- Stochastic computing (Gaines, 1969) — engineered random vs natural phase
- Coherent Sampling (IEEE 1241) — test methodology vs computational paradigm

---

## Phase 1: Critical Fixes (Feb-Mar 2026)

### Test Suite Gaps (from verification report)

**CRITICAL — must fix before any publication:**

1. **Autocorrelation test at multiple lags**
   - Generate 100K phit samples
   - Compute Pearson r at lags 1, 2, 5, 10, 50, 100
   - Assert |r| < 3/sqrt(N) for all lags
   - This is the single most important missing test

2. **Serial independence test**
   - Generate pairs (phit_sample(), phit_sample())
   - Build 2D histogram K×K
   - Chi-squared on contingency table, df=(K-1)²

3. **Fix NIST language**
   - Current: "passes all NIST SP 800-22 statistical tests"
   - Should be: "passes a subset of NIST-inspired tests (monobit, runs, byte distribution)"
   - Or: run full NIST STS (all 15 tests)

4. **Fix Chi-squared critical values**
   - Self-test uses 30.0 — should be 14.07 for df=7
   - phi_adaptive uses df + 2*sqrt(2*df) — approximate, OK
   - phi_uniform uses df + 2*sqrt(df) — too strict
   - Standardize with Wilson-Hilferty: chi2_crit = df * (1 - 2/(9*df) + z*sqrt(2/(9*df)))^3

**HIGH priority:**

5. **Runtime timer resolution detection** — don't hardcode 42ns tick
6. **Cross-platform test** — at least Linux x86_64
7. **Stress test under CPU load** — spawn N background threads
8. **Fix phit_route(0)** — division by zero
9. **Fix phit_prng_range() modulo bias**
10. **Add Makefile** — currently no build system

### Min-Entropy Analysis
- Run NIST SP 800-90B estimators (ea_iid, ea_non_iid) on raw un-hashed samples
- Report min-entropy (H_inf), not just Shannon entropy (H)
- If H_inf < 1.0 per sample, the 1.96 claim needs revision
- This is non-negotiable for any security venue paper

### Fixed-Frequency Experiment
- Pin CPU to single frequency (disable DVFS)
- Re-run entropy measurement
- If entropy drops to ~0: the model needs revision (it's jitter, not phase)
- If entropy stays >1 phit: phase explanation strengthened

---

## Phase 2: Multi-Platform Validation (Mar-Apr 2026)

### Platforms to test
| Platform | How | Cost |
|----------|-----|------|
| Apple M1 Max | Local (done) | €0 |
| Apple M4 | Borrow/buy | — |
| Intel Alder Lake / Raptor Lake | AWS c6i.metal | ~€5/hr |
| AMD Zen 4 | AWS c6a.metal | ~€5/hr |
| ARM Graviton 3 | AWS c6g.metal | ~€3/hr |
| Raspberry Pi 4/5 | Buy (~€60) | €60 |
| RISC-V (SiFive/StarFive) | Buy (~€100) | €100 |

**Total estimated cloud cost: €200-400**

### What to measure per platform
- Timer resolution (actual, not documented)
- Raw phit Shannon entropy per single read
- Raw phit min-entropy per single read (NIST 800-90B)
- Compound sampling (N=2) phit yield
- Routing uniformity Chi-squared
- PRNG throughput (Mbit/s)
- Autocorrelation at lags 1-100

---

## Phase 3: Academic Papers (Apr 2026 → Apr 2027)

### Paper 1: The Foundational Paper
**"Phits: Phase-Bits as Computational Resource in Commodity Processors"**

| Field | Value |
|-------|-------|
| Target | ASPLOS 2027 |
| Deadline | Jul-Oct 2026 (verify asplos-conference.org) |
| Core claim | Phit as unit of phase-derived information; 2-4 phits/read on commodity hardware |
| Required work | Multi-platform data, min-entropy, comparison with jitterentropy |
| Prep time | 4-5 months |
| Co-author | Computer architecture researcher (ETH Zurich, EPFL, TU Delft, UMich) |
| Fallback | HPCA 2027, IEEE Micro Special Issue |
| arXiv | Post BEFORE conference submission (May 2026) |
| Category | cs.AR, cs.PF |

### Paper 2: The Systems Paper
**"Phase-Routed Scheduling: Lock-Free Task Distribution via Clock Domain Phase"**

| Field | Value |
|-------|-------|
| Target | EuroSys 2027 |
| Deadline | Oct 2026 |
| Core claim | Lock-free routing at 28 Mroute/s with zero shared state |
| Required work | Baselines (atomic RR, arc4random, work-stealing), real workloads, 64+ core scaling |
| Prep time | 5-6 months |
| Co-author | OS / parallel computing researcher |
| Fallback | USENIX ATC 2027 (Jan 2027 deadline), SIGOPS OSR |
| arXiv | 2 weeks before deadline |
| Category | cs.OS, cs.DC |

### Paper 3: The Security Paper
**"Phase Entropy: Characterizing Asynchronous Clock Phase as a Hardware Entropy Source"**

| Field | Value |
|-------|-------|
| Target | USENIX Security 2027 |
| Deadline | Jun/Oct 2026 or Feb 2027 (3 cycles) |
| Core claim | Formal entropy analysis, 2-4x jitterentropy yield, cross-platform robustness |
| Required work | Full NIST 800-90B + 800-22 + TestU01 BigCrush + Dieharder, side-channel analysis, adversarial model |
| Prep time | 6-8 months |
| Co-author | **CRITICAL** — cryptography or hardware security |
| Fallback | ACM CCS 2027, CHES 2027, Journal of Cryptographic Engineering |
| arXiv | After submission |
| Category | cs.CR |

### Paper 4: The Architecture Paper
**"RDPHI: An ISA Extension for Direct Clock Phase Readout"**

| Field | Value |
|-------|-------|
| Target | MICRO 2027 |
| Deadline | Apr 2027 |
| Core claim | RDPHI design, FPGA prototype, 100-300x throughput vs software |
| Required work | RTL in Verilog, FPGA prototype with 2+ clock domains, cycle-accurate simulation, area/power |
| Prep time | 8-12 months |
| Co-author | **ESSENTIAL** — VLSI/FPGA researcher with lab access |
| Fallback | ISCA 2027 (Nov 2026), IEEE CAL (4-page letter, rolling) |
| FPGA cost | €200-500 dev board |
| Tools | Yosys (open source) or academic Synopsys/Cadence license via co-author |
| arXiv | After submission |
| Category | cs.AR |

### Paper 5 (Optional): The Theory Paper
**"Temporal Information in Digital Computation: An Extended State Model"**

| Field | Value |
|-------|-------|
| Target | IEEE Transactions on Information Theory |
| Deadline | Rolling |
| Core claim | Formal K-torus model, entropy bounds, impossibility results |
| Required work | Rigorous proofs, connection to Turing models and Kolmogorov complexity |
| Prep time | 8-12 months |
| Co-author | Information theorist / theoretical CS |
| Fallback | ISIT 2027 (5-page, Jan 2027), Theoretical Computer Science journal |
| arXiv | Check IEEE preprint policy before posting |
| Category | cs.IT, cs.CC |

### Priority
1. **Paper 1** (establishes concept) — do this first
2. **Paper 3** (security, forces min-entropy work) — strengthens Paper 1
3. **Paper 2** (systems, most practical) — strongest application
4. **Paper 4** (architecture, needs FPGA) — only with co-author
5. **Paper 5** (theory) — long game, highest prestige if proofs are strong

---

## Phase 4: FPGA Prototype (2027)

### Goal
Validate RDPHI on real hardware with independent clock domains.

### Plan
1. FPGA dev board (Xilinx Artix-7 or similar, ~€200-500)
2. 3 PLLs at incommensurable frequencies → real clock domains
3. Phase comparator in Verilog (~200 gates)
4. Measure: actual phit throughput, quality, gate count, critical path
5. Phase-stochastic ALU: MUX selected by phase register (not LFSR)
6. Benchmark vs LFSR-based stochastic computing

### Quantum-inspired extension
- Phase-weighted stochastic ALU as quantum-inspired classical processor
- Implement quantum-annealing-inspired optimization (phase = temperature)
- Benchmark vs quantum simulators
- Paper: "Phase-weighted classical computation as quantum-inspired stochastic processor"

---

## Phase 5: Ecosystem (2027+)

### Open Source
- Make repo public AFTER arXiv preprint for Paper 1 (May 2026)
- Add CITATION.cff pointing to arXiv
- Add CI (GitHub Actions) for multi-platform testing
- Add CONTRIBUTING.md inviting cross-platform validation
- Promote: Reddit r/programming, HackerNews, RISC-V mailing lists

### Patent Extension
- Within 12 months of UIBM filing: decide on PCT/EPO/USPTO
- PCT covers 153 countries, ~€3000-4000 filing
- Decision depends on: paper acceptance, industry interest, licensing potential

### Industry Contacts
- RISC-V International (for RDPHI proposal)
- Embedded systems vendors (entropy source for IoT without RDRAND)
- Game engine developers (lock-free scheduling)
- Audio DSP companies (phase-weighted modulation for reverb/spatial audio)

---

## Timeline Summary

| Month | Action |
|-------|--------|
| **Feb 2026** | UIBM patent filed. UIBM structure created. Start critical fixes. |
| **Mar 2026** | Autocorrelation + serial independence tests. Min-entropy (800-90B). Contact co-authors. |
| **Apr 2026** | Multi-platform data collection. Start writing Paper 1. |
| **May 2026** | arXiv preprint Paper 1. Repo public. |
| **Jun-Jul 2026** | Submit Paper 1 (ASPLOS 2027). Start Paper 2 baselines. |
| **Aug 2026** | Full NIST + TestU01 + Dieharder for Paper 3. |
| **Sep-Oct 2026** | Submit Paper 2 (EuroSys 2027). Paper 3 writing. |
| **Nov 2026** | Paper 3 → USENIX Security 2027. Start FPGA work (if co-author found). |
| **Jan-Feb 2027** | Paper 4 FPGA prototype. Paper 5 draft. |
| **Mar-Apr 2027** | Submit Paper 4 (MICRO 2027). Submit Paper 5 (IEEE Trans IT). |
| **2027+** | PCT decision. Industry outreach. Ecosystem building. |
