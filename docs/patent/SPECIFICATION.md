# PROVISIONAL PATENT APPLICATION

## Title of Invention

**Method and System for Extracting and Utilizing Computational Phase-Bits (Phits) from Asynchronous Clock Domain Relationships in Digital Processors**

---

## Inventor

**Name:** Alessio Cazzaniga
**Citizenship:** Italian
**Residence:** Varese area, Italy

---

## Cross-Reference to Related Applications

This is a provisional patent application filed under 35 U.S.C. Section 111(b).

---

## Field of the Invention

The present invention relates to digital processor architecture and, more specifically, to methods and systems for extracting usable information from the phase relationships between asynchronous clock domains present in modern multi-clock processors. The invention defines a new unit of information, the "phit" (phase-bit), and provides methods for extracting, accumulating, and utilizing phits for entropy generation, lock-free task coordination, and temporal cryptographic keying.

---

## Background of the Invention

### Problem Statement

Modern digital processors contain multiple independent clock domains operating at different frequencies. For example, the Apple M1 Max processor contains at least three distinct clock domains:

- Performance cores (P-cores) at approximately 3,228 MHz
- Efficiency cores (E-cores) at approximately 2,064 MHz
- System timer at 24 MHz (ARM Generic Timer, cntvct_el0)

These clock domains are asynchronous with respect to each other. The phase relationship between any two clocks changes continuously and deterministically according to their frequency ratio. This phase information is currently treated as noise — something to be eliminated through synchronization barriers, clock domain crossing circuits, and metastability guards.

### Prior Art

**Jitterentropy (Linux Kernel):** The jitterentropy module (Stephan Muller) harvests CPU timing jitter as an entropy source for /dev/random. It executes CPU operations, measures timing variations, and collects least-significant bits as random data. Jitterentropy treats timing variations as random noise to be collected, yielding approximately 1 bit of entropy per measurement. It does not formalize or exploit the structured phase relationship between clock domains, nor does it use phase information for computation beyond entropy harvesting.

**Hardware Random Number Generators (RDRAND/RDSEED, ARM RNDR):** Intel and ARM provide dedicated hardware circuits for random number generation. These require dedicated silicon area and are not available on all processors, particularly low-cost embedded and IoT devices.

**Round-Robin and Shared-State Scheduling:** Conventional task scheduling in multi-processor systems relies on shared counters, atomic operations, or mutex-protected state to distribute work among workers. These mechanisms create contention points that limit scalability.

### Deficiencies of Prior Art

1. No prior art formalizes the phase relationship between asynchronous clock domains as a computational resource.
2. Jitterentropy extracts approximately 1 bit per measurement; the present invention extracts 2-4 phits per measurement through compound sampling.
3. Hardware RNGs require dedicated silicon; the present invention uses existing clock infrastructure.
4. Lock-free scheduling via phase has not been previously proposed.
5. Temporal cryptographic keying based on clock frequency relationships has not been previously described.

---

## Summary of the Invention

The present invention introduces a computational framework called "Triphase Computation" that treats the phase relationship between asynchronous clock domains as an exploitable information resource. The framework defines:

1. **The phit (phase-bit):** A unit of information extracted from the phase relationship between two or more asynchronous clock domains.

2. **An extended computational state:** Sigma(t) = (S(t), Phi(t)), where S(t) represents the conventional spatial state (register contents, memory) and Phi(t) represents the phase vector — the set of phase relationships between all clock domain pairs at time t.

3. **Methods for extracting phits** from existing processor hardware without modification, using compound sampling of timer values and workload timing deltas.

4. **Three computational paradigms** for utilizing phits:
   - Phase-Gated Computation: executing operations only when clock phases align at specific points
   - Phase-Weighted Computation: using phase values as arithmetic coefficients
   - Phase-Encoded Computation: multiplexing N values in a single register via time-division based on phase

5. **Three practical applications** demonstrated on commodity hardware:
   - Phase-based pseudo-random number generation with quality passing NIST SP 800-22 statistical tests
   - Lock-free task scheduling using phase instead of shared counters
   - Phase-gated encryption where the cryptographic key is derived from clock frequency relationships at a specific moment in time

6. **A proposed ISA extension** (RDPHI instruction) for direct hardware phase readout, projected to achieve 100-300x throughput improvement over the software extraction method.

---

## Detailed Description of the Invention

### 1. Theoretical Foundation

#### 1.1 Extended State Model

A conventional processor's state at time t is described by its spatial state S(t) — the contents of registers, memory, and flags. The present invention extends this model:

```
Sigma(t) = (S(t), Phi(t))
```

where Phi(t) is the phase vector:

```
Phi(t) = (phi_1(t), phi_2(t), ..., phi_K(t))
```

For K clock domains with frequencies f_1, f_2, ..., f_K, each phase component is:

```
phi_i(t) = f_i * t mod 1
```

The phase difference between any two clock domains i and j is:

```
delta_phi_ij(t) = (f_i - f_j) * t mod 1
```

This phase difference oscillates at the beat frequency |f_i - f_j| and is deterministic for known frequencies and time, but appears random to an observer who does not know the exact frequencies.

#### 1.2 Information Content

The Shannon entropy of the phase channel between two asynchronous clocks depends on the measurement resolution. With a timer of resolution R and a workload of duration D ticks, the number of distinguishable phase states is approximately D/R, yielding:

```
H = log2(D/R) bits per measurement
```

For the Apple M1 Max with a 24 MHz timer (R = 41.67 ns) and a calibrated workload of approximately 800 ns duration, the measured entropy is approximately 1.96 phits per single read, and 4.06 phits per compound read (N=2).

#### 1.3 The Triad Architecture

The fundamental unit of triphase computation is a triad of three asynchronous clocks:

- **Clock Alpha:** The first oscillator (e.g., P-core clock)
- **Clock Beta:** The second oscillator (e.g., system timer)
- **Clock Observer:** The third clock that samples the phase relationship between Alpha and Beta

Two clocks generate a beat signal; the third observes and computes based on the phase relationship.

### 2. Method of Phit Extraction (Software)

#### 2.1 Basic Phit Sampling

The method for extracting a single phit sample from existing hardware comprises:

**Step 1:** Execute a calibrated computational workload. The workload must have execution time that varies with the phase relationship between the CPU clock and the timer clock. A preferred embodiment uses a linear congruential generator (LCG) loop:

```c
volatile uint64_t x = SEED;
for (int i = 0; i < N_ITERATIONS; i++) {
    x = x * 6364136223846793005ULL + 1442695040888963407ULL;
}
```

The volatile qualifier prevents compiler optimization, ensuring the workload actually executes. The number of iterations N_ITERATIONS is chosen to produce a workload duration of approximately 200-800 nanoseconds, sufficient to span multiple timer ticks.

**Step 2:** Read the system timer immediately after the workload completes.

**Step 3:** Combine the timer's least-significant bits (which are provably uniform with Chi-squared = 0.39) with the workload-dependent timing information:

```c
uint64_t t = read_timer_ns();
uint32_t key = (t & 0x3) | (((uint32_t)(t >> 2) ^ (uint32_t)x) << 2);
```

The two least-significant bits of the timer provide uniform entropy. The upper bits, XORed with the workload result, capture the phase-dependent timing variation.

**Step 4:** Apply a hash function to spread the combined value across the full output range:

```c
key *= 2654435761u;    // Knuth multiplicative hash
key ^= key >> 16;
key *= 0x85ebca6bu;
key ^= key >> 13;
```

#### 2.2 Compound Phit Sampling

To increase the number of phits per sample, the method employs compound sampling with N sequential reads:

**Step 1:** For each read i = 0 to N-1:
  - Vary the workload seed using the read index to prevent identical execution patterns
  - Execute the calibrated workload
  - Read the timer
  - Combine timer LSBs with workload result as in basic sampling
  - Accumulate into a compound key using rotation and XOR

**Step 2:** Apply a final hash to the compound key.

The compound method with N=2 reads produces approximately 4.06 phits, compared to 1.96 phits for a single read. The optimal value of N depends on the target application's throughput requirements versus entropy needs.

#### 2.3 Timer Least-Significant Bit Uniformity

A key discovery of this invention is that the least-significant bits of the system timer exhibit provably uniform distribution. On the Apple M1 Max, the 2 LSBs of clock_gettime_nsec_np(CLOCK_UPTIME_RAW) produce a Chi-squared value of 0.39 across 4 bins (critical value 7.81 at p=0.05, 3 degrees of freedom), confirming uniformity. This uniformity arises from the asynchronous relationship between the timer clock domain and the CPU clock domain.

#### 2.4 Non-Stationarity Finding

During development, it was discovered that the distribution of timing deltas is non-stationary due to dynamic CPU frequency scaling. A CDF-based routing method calibrated at one CPU frequency produces biased results when the CPU frequency changes. The compound sampling method described in Section 2.2 is robust to this non-stationarity because it combines uniform timer LSBs (which are frequency-independent) with the timing delta.

### 3. Phit Entropy Pool and PRNG

#### 3.1 Entropy Pool Structure

The invention includes an entropy accumulation pool consisting of:

- A 256-bit state divided into 4 lanes of 64 bits each
- A monotonic mix counter
- A bits-collected estimator

#### 3.2 Pool Feeding

When a phit sample is fed into the pool:

1. The mix counter increments
2. The sample is combined with the counter using the golden ratio constant (0x9E3779B97F4A7C15) to prevent cycles
3. The combined value undergoes SplitMix64 mixing:
   ```
   z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9
   z = (z ^ (z >> 27)) * 0x94D049BB133111EB
   z = z ^ (z >> 31)
   ```
4. The result is XORed into the current pool lane
5. Adjacent lanes are cross-mixed using 64-bit rotation by 17 positions

#### 3.3 Pool Extraction

To extract a 64-bit random value:

1. Four fresh phit harvests are performed (8 pool feeds total — each harvest feeds both the raw timer value and the timer XORed with the workload result)
2. All four pool lanes are combined using XOR with different rotation amounts (13, 29, 43 positions)
3. Forward security is maintained by mutating two pool lanes with rotated versions of the output

#### 3.4 PRNG Interface

The PRNG provides:
- `prng_u64()`: 64-bit unsigned random integer
- `prng_double()`: Double-precision floating point in [0, 1) using the upper 53 bits
- `prng_range(max)`: Uniform integer in [0, max)
- `prng_fill(buffer, length)`: Fill a byte buffer with random data

#### 3.5 Quality Results

On Apple M1 Max, the PRNG passes all tested NIST SP 800-22 statistical tests:

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

1. When a task arrives for dispatch, extract a phit sample using compound sampling (N=2)
2. Compute: `worker_id = phit_sample % num_workers`
3. Route the task to the selected worker

No mutex, no atomic counter, and no shared memory is required. Each thread independently computes the same routing function, and the phase relationship between their clocks ensures uniform distribution.

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

The invention provides a cryptographic method where the encryption key is not stored in memory but is derived from:
- The frequencies of three or more clock domains (known to authorized parties)
- The exact time of encryption

The keystream for byte position i at time t is derived from the phase vector:

```
phi_k(t) = f_k * t mod 1, for k = 1, 2, 3
keystream[i] = hash(phi_1 * 256 + phi_2 * 65536 + phi_3 * 16777216 + i * golden_ratio)
```

Encryption and decryption are symmetric (XOR with keystream).

#### 5.2 Temporal Sensitivity

A timing error of 1 microsecond produces completely different output (garbage decryption). This property arises because the clock frequencies (GHz range) cause phase vectors to change completely within nanoseconds.

#### 5.3 Phase-Locked Access Windows

The invention further provides a method for restricting decryption to specific phase windows — time intervals where the relative phase between two clock domains falls within a specified range. Outside this window, decryption produces incorrect output.

### 6. Proposed ISA Extension (RDPHI)

#### 6.1 Instruction Definition

The invention proposes three new processor instructions for the RISC-V ISA custom extension space:

```
RDPHI  rd, rs1, rs2   — Read phase between clock domains rs1 and rs2 into rd
PHIGATE rd, rs1        — Gate execution on phase condition in rs1
PHIWEIGHT rd, rs1      — Return phase as fixed-point weight in rd
```

#### 6.2 Hardware Implementation

The RDPHI instruction is implemented via a phase comparator circuit:
- A flip-flop samples the counter of clock domain A on the rising edge of clock domain B
- The sampled value represents the instantaneous phase relationship
- The circuit requires approximately 200 logic gates

#### 6.3 Projected Performance

| Metric | Software (current) | Hardware (RDPHI) | Improvement |
|--------|-------------------|-------------------|-------------|
| Time per read | ~100 ns | ~0.3 ns (1 cycle) | ~300x |
| Phits per read | ~2 | ~8-16 | ~4-8x |
| Throughput | 24.6 Mphit/s | ~3-10 Gphit/s | ~100-400x |
| CPU overhead | Workload + timer read | 1 instruction | ~100x |

### 7. Portable Software Library (libphit.h)

The invention includes a header-only C library that provides portable phit extraction across platforms:

- **macOS (ARM64):** Uses clock_gettime_nsec_np(CLOCK_UPTIME_RAW)
- **macOS (x86):** Uses clock_gettime_nsec_np(CLOCK_UPTIME_RAW)
- **Linux (ARM64/x86):** Uses clock_gettime(CLOCK_MONOTONIC_RAW)
- **FreeBSD:** Uses clock_gettime(CLOCK_MONOTONIC)

The library provides self-test functionality that validates hash determinism, PRNG output quality (monobit test), routing uniformity, and timer advancement.

---

## Claims

### Independent Claims

**Claim 1.** A method for extracting information from the phase relationship between asynchronous clock domains in a digital processor, comprising:
(a) executing a calibrated computational workload on the processor;
(b) reading a system timer value after completion of the workload;
(c) combining the least-significant bits of the timer value with a representation of the workload execution to produce a phase-dependent value;
(d) applying a hash function to the phase-dependent value to produce a phit (phase-bit) sample.

**Claim 2.** A method for compound phit sampling, comprising performing the method of Claim 1 multiple times with varied workload seeds, and accumulating the results using rotation and exclusive-or operations to produce a compound phit sample with higher information content than a single sample.

**Claim 3.** A method for lock-free task routing in a multi-processor system, comprising:
(a) extracting a phit sample according to the method of Claim 1 or Claim 2;
(b) computing a worker identifier as the phit sample modulo the number of available workers;
(c) routing the task to the identified worker without accessing any shared state, mutex, or atomic counter.

**Claim 4.** A pseudo-random number generator comprising:
(a) an entropy pool of N lanes of M bits each;
(b) a feeding mechanism that mixes phit samples extracted according to the method of Claim 1 into the pool using multiplicative hashing and cross-lane rotation;
(c) an extraction mechanism that combines all pool lanes using rotated exclusive-or and mutates the pool state after extraction for forward security.

**Claim 5.** A method for phase-gated encryption, comprising:
(a) deriving a keystream from the phase vector Phi(t) = (f_1*t mod 1, f_2*t mod 1, ..., f_K*t mod 1) at a specific time t, where f_1, f_2, ..., f_K are the frequencies of K asynchronous clock domains;
(b) encrypting a plaintext message by exclusive-or with the keystream;
(c) wherein decryption requires knowledge of both the exact clock frequencies and the exact time of encryption, and timing errors on the order of microseconds produce incorrect decryption.

**Claim 6.** A processor instruction (RDPHI) that reads the instantaneous phase relationship between two specified clock domains and stores the result in a general-purpose register, implemented via a phase comparator circuit comprising a flip-flop that samples the counter of one clock domain on the edge of another clock domain.

### Dependent Claims

**Claim 7.** The method of Claim 1, wherein the least-significant 2 bits of the timer value are used as a uniform entropy source, and the uniformity is validated by a Chi-squared test with a value below the critical threshold for the number of bins.

**Claim 8.** The method of Claim 2, wherein the number of reads N=2 is used as the optimal trade-off between phit yield and throughput.

**Claim 9.** The method of Claim 5, further comprising phase-locked access windows wherein decryption is only possible when the relative phase between a specified pair of clock domains falls within a predetermined range.

**Claim 10.** The PRNG of Claim 4, wherein the pool consists of 4 lanes of 64 bits each (256 bits total), and cross-lane mixing uses rotation by 17 bit positions.

**Claim 11.** A portable software library implementing the methods of Claims 1 through 5 as a header-only C library, with platform-specific timer backends for ARM, x86, and RISC-V architectures.

---

## Abstract

A method and system for extracting and utilizing computational phase-bits ("phits") from the phase relationships between asynchronous clock domains in digital processors. The invention treats clock phase information — currently discarded as noise — as an exploitable computational resource. A phit is defined as the unit of information derived from the phase relationship between two asynchronous clocks. The invention provides methods for software-based phit extraction achieving 1.96 phits per single read and 4.06 phits per compound read on commodity hardware (Apple M1 Max). Three applications are demonstrated: (1) a pseudo-random number generator passing NIST SP 800-22 tests at 181 Mbit/s throughput; (2) lock-free task routing at 28 million routes per second with zero shared state; and (3) phase-gated encryption where the key exists only as a temporal relationship between clock frequencies. A hardware ISA extension (RDPHI) is proposed for direct phase readout, projected to achieve 100-300x throughput improvement over the software method.

---

## Drawings Description

The following drawings would accompany this specification:

**FIG. 1** — Block diagram of the Triad Architecture showing three asynchronous clock domains (Alpha, Beta, Observer) and their phase relationships.

**FIG. 2** — Flowchart of the basic phit sampling method (Steps 1-4 of Section 2.1).

**FIG. 3** — Flowchart of the compound phit sampling method (Section 2.2).

**FIG. 4** — Block diagram of the entropy pool structure showing 4 lanes, feed mechanism, and extraction mechanism (Section 3).

**FIG. 5** — Diagram of the RDPHI hardware phase comparator circuit (Section 6.2).

**FIG. 6** — Table of experimental results on Apple M1 Max (Sections 3.5 and 4.2).

**FIG. 7** — Diagram showing phase-gated encryption and decryption flow (Section 5).

*Note: For a provisional patent application, formal drawings are not required. The descriptions above indicate the drawings that would accompany the non-provisional application.*

---

## Filing Notes

- This specification is intended for filing as a US Provisional Patent Application
- Filing fee: $163 (micro entity)
- File at: USPTO Patent Center (patentcenter.uspto.gov)
- Form: SB/16 (Provisional Application Cover Sheet)
- The provisional establishes priority date; convert to non-provisional within 12 months
- No formal claims examination occurs for provisional applications
- The claims section above is included for completeness and to support the non-provisional conversion

---

*Specification prepared for: Alessio Cazzaniga*
*Date of preparation: February 2, 2026*
