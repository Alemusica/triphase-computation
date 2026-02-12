# Method and System for Extracting and Utilizing Computational Phase-Bits (Phits) from Asynchronous Clock Domain Relationships in Digital Processors

**Titolo (IT):** Metodo e sistema per l'estrazione e l'utilizzo di bit di fase computazionali (phit) dalle relazioni tra domini di clock asincroni nei processori digitali

---

## FIELD OF THE INVENTION / CAMPO DELL'INVENZIONE

The present invention relates to digital processor architecture and, more specifically, to methods and systems for extracting usable information from the phase relationships between asynchronous clock domains present in modern multi-clock processors. The invention defines a new unit of information, the "phit" (phase-bit), and provides methods for extracting, accumulating, and utilizing phits for entropy generation, lock-free task coordination, and temporal cryptographic keying.

---

## BACKGROUND OF THE INVENTION / CONTESTO DELL'INVENZIONE

### Problem Statement / Problema Tecnico

Modern digital processors contain multiple independent clock domains operating at different frequencies. For example, the Apple M1 Max processor contains at least three distinct clock domains:

- Performance cores (P-cores) at approximately 3,228 MHz
- Efficiency cores (E-cores) at approximately 2,064 MHz
- System timer at 24 MHz (ARM Generic Timer, cntvct_el0)

These clock domains are asynchronous with respect to each other. The phase relationship between any two clocks changes continuously and deterministically according to their frequency ratio. This phase information is currently treated as noise — something to be eliminated through synchronization barriers, clock domain crossing circuits, and metastability guards.

The present invention recognizes that this phase information is not noise but an exploitable computational resource with measurable information content.

### Prior Art / Stato della Tecnica

**Jitterentropy (Linux Kernel):** The jitterentropy module (Stephan Muller) harvests CPU timing jitter as an entropy source for /dev/random. It executes CPU operations, measures timing variations, and collects least-significant bits as random data. Jitterentropy treats timing variations as random noise to be collected, yielding approximately 1 bit of entropy per measurement (conservative SP 800-90B estimate: 1/3 bit per measurement). It does not formalize or exploit the structured phase relationship between clock domains, nor does it use phase information for computation beyond entropy harvesting.

**Hardware Random Number Generators (RDRAND/RDSEED, ARM RNDR):** Intel and ARM provide dedicated hardware circuits for random number generation based on thermal noise in self-timed circuits. These require dedicated silicon area and are not available on all processors, particularly low-cost embedded and IoT devices.

**Clock Computing Machines (US10498528B2, Kwok, 2019):** This patent describes computation using time states within a single cyclic counter (clock machine) with N discrete time states. Clock machines use deterministic modulo arithmetic on single-clock time state sequences. The present invention differs fundamentally in that it exploits the phase relationship between multiple asynchronous clock domains with continuous (not discrete) phase values, and provides three distinct computational paradigms (gated, weighted, encoded) not described in clock machines.

**RISC-V Entropy Source Interface (Neumann et al., 2022):** The RISC-V ISA defines an entropy source extension using dual rationally-related synthesized clock signals for noise extraction, compliant with SP800-90B and FIPS 140-3. This interface provides entropy for random number generation only. The present invention's proposed RDPHI instruction returns phase information for general-purpose computation (gating, weighting, encoding), not solely for entropy extraction.

**Ring Oscillator TRNGs:** True Random Number Generators based on phase jitter between ring oscillators in FPGAs/ASICs. These are hardware-only implementations requiring dedicated circuits, and focus on jitter magnitude rather than structured phase relationships between independent clock domains.

**Round-Robin and Shared-State Scheduling:** Conventional task scheduling in multi-processor systems relies on shared counters, atomic operations, or mutex-protected state to distribute work among workers. These mechanisms create contention points that limit scalability.

### Deficiencies of Prior Art / Carenze della Tecnica Nota

1. No prior art formalizes the phase relationship between asynchronous clock domains as a general computational resource with three paradigms (gated, weighted, encoded).
2. Jitterentropy extracts approximately 1 bit per measurement; the present invention extracts 2-4 phits per measurement through compound sampling.
3. Clock Machines (US10498528B2) use a single clock with discrete time states; the present invention uses multiple asynchronous clocks with continuous phase relationships.
4. Hardware RNGs require dedicated silicon; the present invention uses existing clock infrastructure on commodity processors.
5. Lock-free scheduling via phase without any shared state has not been previously proposed.
6. RISC-V entropy source returns entropy only; the present invention's RDPHI returns phase for computation.

---

## SUMMARY OF THE INVENTION / SOMMARIO DELL'INVENZIONE

The present invention introduces a computational framework called "Triphase Computation" that treats the phase relationship between asynchronous clock domains as an exploitable information resource. The framework defines:

1. The phit (phase-bit): A unit of information extracted from the phase relationship between two or more asynchronous clock domains.

2. An extended computational state: Sigma(t) = (S(t), Phi(t)), where S(t) represents the conventional spatial state (register contents, memory) and Phi(t) represents the phase vector — the set of phase relationships between all clock domain pairs at time t.

3. Methods for extracting phits from existing processor hardware without modification, using compound sampling of timer values and workload timing deltas.

4. Three computational paradigms for utilizing phits:
   - Phase-Gated Computation: executing operations only when clock phases align at specific points
   - Phase-Weighted Computation: using phase values as arithmetic coefficients
   - Phase-Encoded Computation: multiplexing N values in a single register via time-division based on phase

5. Three practical applications demonstrated on commodity hardware:
   - Phase-based pseudo-random number generation passing four NIST SP 800-22-inspired statistical tests (Monobit, Runs, Byte Distribution, Per-Bit Entropy)
   - Lock-free task scheduling using phase instead of shared counters
   - Phase-gated encryption where the cryptographic key is derived from clock frequency relationships at a specific moment in time

6. A proposed ISA extension (RDPHI instruction) for direct hardware phase readout, projected to achieve 100-300x throughput improvement over the software extraction method.

---

## DETAILED DESCRIPTION OF THE INVENTION / DESCRIZIONE DETTAGLIATA DELL'INVENZIONE

### 1. Theoretical Foundation

#### 1.1 Extended State Model

A conventional processor's state at time t is described by its spatial state S(t) — the contents of registers, memory, and flags. The present invention extends this model:

Sigma(t) = (S(t), Phi(t))

where Phi(t) is the phase vector:

Phi(t) = (phi_1(t), phi_2(t), ..., phi_K(t))

For K clock domains with frequencies f_1, f_2, ..., f_K, each phase component is:

phi_i(t) = f_i * t mod 1

The phase difference between any two clock domains i and j is:

delta_phi_ij(t) = (f_i - f_j) * t mod 1

This phase difference oscillates at the beat frequency |f_i - f_j| and is deterministic for known frequencies and time, but appears random to an observer who does not know the exact frequencies and observation time.

#### 1.2 Information Content

The Shannon entropy of the phase channel between two asynchronous clocks depends on the measurement resolution. With a timer of resolution R and a workload of duration D ticks, the number of distinguishable phase states is approximately D/R, yielding:

H = log2(D/R) bits per measurement

For the Apple M1 Max with a 24 MHz timer (R = 41.67 ns) and a calibrated workload of approximately 800 ns duration, the measured entropy is approximately 1.96 phits per single read, and 4.06 phits per compound read (N=2).

Mathematical foundation: When clock frequencies are incommensurable (their ratio is irrational), Weyl's Equidistribution Theorem guarantees that phase samples are equidistributed on the unit interval. The Three-Distance Theorem (Steinhaus) constrains the sampling structure to at most 3 distinct gap sizes. The Kolmogorov-Sinai entropy of the deterministic rotation is zero; the extractable entropy comes from the hardware jitter in workload execution, amplified by the phase structure.

#### 1.3 The Triad Architecture

The fundamental unit of triphase computation is a triad of three asynchronous clocks:

- Clock Alpha: The first oscillator (e.g., P-core clock)
- Clock Beta: The second oscillator (e.g., E-core clock)
- Clock Observer: The third clock that samples the phase relationship between Alpha and Beta (e.g., system timer)

Two clocks generate a beat signal; the third observes and computes based on the phase relationship. This asymmetry is fundamental: neither Alpha nor Beta can observe their own phase relationship — only the Observer can.

### 2. Method of Phit Extraction (Software)

#### 2.1 Basic Phit Sampling

The method for extracting a single phit sample from existing hardware comprises:

Step 1: Execute a calibrated computational workload. The workload must have execution time that varies with the phase relationship between the CPU clock and the timer clock. A preferred embodiment uses a linear congruential generator (LCG) loop with volatile qualifier to prevent compiler optimization and N_ITERATIONS chosen to produce a workload duration of approximately 200-800 nanoseconds.

Step 2: Read the system timer immediately after the workload completes.

Step 3: Combine the timer's least-significant bits (which are provably uniform with Chi-squared = 0.39) with the workload-dependent timing information. The two least-significant bits of the timer provide uniform entropy. The upper bits, XORed with the workload result, capture the phase-dependent timing variation.

Step 4: Apply a hash function to spread the combined value across the full output range.

#### 2.2 Compound Phit Sampling

To increase the number of phits per sample, the method employs compound sampling with N sequential reads:

Step 1: For each read i = 0 to N-1: vary the workload seed using the read index, execute the calibrated workload, read the timer, combine timer LSBs with workload result, and accumulate into a compound key using rotation and XOR.

Step 2: Apply a final hash to the compound key.

The compound method with N=2 reads produces approximately 4.06 phits, compared to 1.96 phits for a single read. The optimal value of N=2 provides the best trade-off between phit yield and throughput.

#### 2.3 Timer Least-Significant Bit Uniformity

A key discovery of this invention is that the least-significant bits of the system timer exhibit provably uniform distribution. On the Apple M1 Max, the 2 LSBs of clock_gettime_nsec_np(CLOCK_UPTIME_RAW) produce a Chi-squared value of 0.39 across 4 bins (critical value 7.81 at p=0.05, 3 degrees of freedom), confirming uniformity. This uniformity arises from the asynchronous relationship between the timer clock domain and the CPU clock domain.

#### 2.4 Non-Stationarity and Robustness

During development, it was discovered that the distribution of timing deltas is non-stationary due to dynamic CPU frequency scaling. The compound sampling method described in Section 2.2 is robust to this non-stationarity because it combines uniform timer LSBs (which are frequency-independent) with the timing delta. This was validated empirically: the CDF-based routing method calibrated at one frequency produces biased results when frequency changes, while compound sampling maintains uniformity.

### 3. Phit Entropy Pool and PRNG

#### 3.1 Entropy Pool Structure

The invention includes an entropy accumulation pool consisting of a 256-bit state divided into 4 lanes of 64 bits each, a monotonic mix counter, and a bits-collected estimator.

#### 3.2 Pool Feeding

When a phit sample is fed into the pool: (1) the mix counter increments; (2) the sample is combined with the counter using the golden ratio constant to prevent cycles; (3) the combined value undergoes SplitMix64 mixing; (4) the result is XORed into the current pool lane; (5) adjacent lanes are cross-mixed using 64-bit rotation by 17 positions.

#### 3.3 Pool Extraction

To extract a 64-bit random value: (1) four fresh phit harvests are performed; (2) all four pool lanes are combined using XOR with different rotation amounts; (3) forward security is maintained by mutating two pool lanes with rotated versions of the output.

#### 3.4 PRNG Quality Results

On Apple M1 Max, the PRNG passes four NIST SP 800-22-inspired statistical tests:

| Test | Result | Metric |
|------|--------|--------|
| Monobit | PASS | Z-score < 3.29 |
| Runs | PASS | Z-score < 3.29 |
| Byte Distribution | PASS | Chi-squared = 221.7 (< 310 critical value) |
| Per-Bit Entropy | PASS | 64.0/64.0 bits (100%) |
| Throughput | 181 Mbit/s | (22.6 MB/s) |

### 4. Lock-Free Task Routing via Phits

#### 4.1 Method

The invention provides a method for distributing tasks to worker threads without shared state:
(1) When a task arrives for dispatch, extract a phit sample using compound sampling (N=2).
(2) Compute: worker_id = phit_sample modulo num_workers.
(3) Route the task to the selected worker.

No mutex, no atomic counter, and no shared memory is required. Each thread independently computes the routing function, and the phase relationship between their clocks ensures uniform distribution.

#### 4.2 Results

On Apple M1 Max with 8 workers and 100,000 tasks:

| Metric | Value |
|--------|-------|
| Chi-squared | 7.8 (uniform threshold: 14.07) |
| Routing throughput | 28 Mroute/s |
| Maximum load imbalance | 2.5% |
| Shared state required | None |
| Lock contention | Zero |

### 5. Phase-Gated Encryption

#### 5.1 Concept

The invention provides an encryption method where the key is derived from the frequencies of three or more clock domains (known to authorized parties) and the exact time of encryption. The keystream for byte position i at time t is derived from the phase vector at time t. Encryption and decryption are symmetric (XOR with keystream).

#### 5.2 Temporal Sensitivity

A timing error of 1 microsecond produces completely different output. This property arises because the clock frequencies (GHz range) cause phase vectors to change completely within nanoseconds.

#### 5.3 Phase-Locked Access Windows

The invention further provides a method for restricting decryption to specific phase windows — time intervals where the relative phase between two clock domains falls within a specified range.

### 6. Proposed ISA Extension (RDPHI)

#### 6.1 Instruction Definition

The invention proposes three new processor instructions for the RISC-V ISA custom extension space:

RDPHI rd, rs1, rs2 — Read phase between clock domains rs1 and rs2 into rd
PHIGATE rd, rs1 — Gate execution on phase condition in rs1
PHIWEIGHT rd, rs1 — Return phase as fixed-point weight in rd

#### 6.2 Hardware Implementation

The RDPHI instruction is implemented via a phase comparator circuit: a flip-flop samples the counter of clock domain A on the rising edge of clock domain B. The sampled value represents the instantaneous phase relationship. The circuit requires approximately 200 logic gates.

#### 6.3 Projected Performance

| Metric | Software (current) | Hardware (RDPHI) | Improvement |
|--------|-------------------|-------------------|-------------|
| Time per read | ~100 ns | ~0.3 ns (1 cycle) | ~300x |
| Phits per read | ~2 | ~8-16 | ~4-8x |
| Throughput | 24.6 Mphit/s | ~3-10 Gphit/s | ~100-400x |
| CPU overhead | Workload + timer read | 1 instruction | ~100x |

### 7. Portable Software Library (libphit.h)

The invention includes a header-only C library (366 lines, MIT license) that provides portable phit extraction across platforms:

- macOS (ARM64): Uses clock_gettime_nsec_np(CLOCK_UPTIME_RAW)
- macOS (x86): Uses clock_gettime_nsec_np(CLOCK_UPTIME_RAW)
- Linux (ARM64/x86): Uses clock_gettime(CLOCK_MONOTONIC_RAW)
- FreeBSD: Uses clock_gettime(CLOCK_MONOTONIC)

The library provides self-test functionality that validates hash determinism, PRNG output quality (monobit test), routing uniformity, and timer advancement.

---

## EXPERIMENTAL VALIDATION / VALIDAZIONE SPERIMENTALE

### Study 1: Phit Extraction on Apple M1 Max

Five experimental programs (phase_extract.c, phase_extract_v2.c, phi_exploit.c, phi_uniform.c, phi_adaptive.c) were developed to characterize phit extraction on the Apple M1 Max (10-core CPU, 32-core GPU, 64 GB unified memory). Key findings:

1. Timer LSBs (2 bits) are provably uniform: Chi-squared = 0.39 (critical: 7.81)
2. Raw phit capacity: 1.96 phits per single read
3. Compound sampling (N=2): 4.06 phits per read
4. Throughput: 24.6 Mphit/s
5. Non-stationarity discovered and solved via compound sampling

### Study 2: PRNG Quality

The phit-based PRNG was tested against four statistical tests inspired by NIST SP 800-22 (Monobit, Runs, Byte Distribution, Per-Bit Entropy) on 10 million 64-bit samples. All four tests passed, with per-bit entropy measured at 100% (64.0/64.0 bits).

### Study 3: Lock-Free Scheduler

100,000 tasks routed to 8 workers without shared state. Chi-squared uniformity: 7.8 (threshold 14.07). Maximum load imbalance: 2.5%. Zero lock contention. Throughput: 28 Mroute/s.

---

## INDUSTRIAL APPLICABILITY / APPLICABILITÀ INDUSTRIALE

The invention has immediate industrial applicability in:

1. Embedded systems and IoT devices lacking hardware random number generators, where the software phit extraction method provides high-quality entropy from existing clocks.
2. High-performance computing, where lock-free task routing via phase eliminates contention in parallel workloads.
3. Digital audio processing, where phase-weighted modulation of DSP parameters provides natural decorrelation and dithering.
4. Cryptographic systems requiring time-bound access controls via phase-locked access windows.
5. Processor design, where the proposed RDPHI instruction adds phase computation capability at minimal silicon cost (~200 gates).

---

## PRIOR ART REFERENCES / RIFERIMENTI ALLO STATO DELLA TECNICA

1. Muller, S. "CPU Time Jitter Based Non-Physical True Random Number Generator." jitterentropy-library, GitHub. BSD/GPLv2.
2. Intel Corporation. "Intel Digital Random Number Generator (DRNG) Software Implementation Guide." 2021.
3. Kwok, M. "Clock Computing Machines." US Patent US10498528B2, 2019.
4. Neumann, M. et al. "Development of the RISC-V Entropy Source Interface." Journal of Cryptographic Engineering, 2022.
5. Gaines, B.R. "Stochastic Computing Systems." Advances in Information Systems Science, Vol. 2, 1969.
6. Madhavan, A., Sherwood, T., Strukov, D. "Race Logic: A hardware acceleration for dynamic programming algorithms." ISCA 2014.
7. NIST SP 800-22. "A Statistical Test Suite for Random and Pseudorandom Number Generators for Cryptographic Applications." 2010.
8. NIST SP 800-90B. "Recommendation for the Entropy Sources Used for Random Bit Generation." 2018.

---

*Specification prepared for: Alessio Cazzaniga*
*Date of preparation: February 12, 2026*
