# Reddit Post Draft — Triphase Computation

## Target subreddits (pick 1-2 to start)
- r/programming (1.1M+ members, general audience)
- r/compsci (500K+, theoretical angle)
- r/netsec (500K+, security/crypto angle)
- r/hardware (200K+, ISA extension angle)

---

## Title options (pick one)

1. "Your CPU has been throwing away free entropy — I found how to catch it"
2. "I extracted usable randomness from the phase relationship between async clock domains — no hardware mod needed"
3. "Lock-free task routing with zero shared state: using clock phase instead of counters"

---

## Post body (for r/programming, adapt for others)

---

**TL;DR:** Every modern CPU has multiple asynchronous clock domains. The phase relationship between them changes continuously and contains extractable information. I wrote a C library that harvests this "phase entropy" on commodity hardware — no special instructions needed. It passes NIST-inspired statistical tests, does lock-free scheduling with zero shared state, and I think it opens a new computational paradigm. Patent pending, source available (BSL 1.1 — free for non-production use, Apache 2.0 in 2030).

---

### The insight

Your CPU has at least 3 independent clocks: performance cores (~3.2 GHz), efficiency cores (~2 GHz), and a system timer (24 MHz). These are asynchronous — their phase relationship shifts continuously.

Current engineering treats this as noise. Clock domain crossing circuits, synchronization barriers, metastability guards — all designed to *eliminate* phase information.

I asked: what if it's not noise? What if it's a resource?

### What I built

**libphit.h** — a header-only C library (366 lines, MIT) that extracts "phits" (phase-bits) from existing hardware:

```c
#define LIBPHIT_IMPLEMENTATION
#include "libphit.h"

// Extract a phase sample — no special hardware needed
phit_sample_t sample = phit_compound_sample(2);
// sample.key contains ~4 phits of phase-derived information

// Use it for RNG
phit_prng_t rng;
phit_prng_init(&rng);
uint64_t random_val = phit_prng_next(&rng);

// Or for lock-free routing — zero shared state
int worker = phit_route(num_workers);
```

### How it works

1. Execute a calibrated workload (tiny LCG loop, ~800ns)
2. Read the system timer immediately after
3. The timer's 2 LSBs are provably uniform (Chi-squared = 0.39, threshold 7.81) due to the async clock relationship
4. Combine timer LSBs + workload timing delta + hash → phit sample
5. Compound sampling (N=2 reads) gives ~4.06 phits per sample

The math: when clock frequencies are incommensurable (irrational ratio), Weyl's Equidistribution Theorem guarantees uniform phase distribution. The Three-Distance Theorem constrains the sampling structure. The extractable entropy comes from hardware jitter amplified by phase structure.

### Results on Apple M1 Max

**PRNG quality (10M samples):**
- Monobit test: PASS
- Runs test: PASS
- Byte distribution: Chi-squared = 221.7 (< 310 critical)
- Per-bit entropy: 64.0/64.0 bits (100%)
- Throughput: 181 Mbit/s

**Lock-free scheduler (100K tasks, 8 workers):**
- Uniformity: Chi-squared = 7.8 (threshold: 14.07)
- Max load imbalance: 2.5%
- Shared state: **none**
- Lock contention: **zero**
- Throughput: 28 Mroute/s

### The bigger picture

I think this is a new computational primitive. I define three paradigms:

- **Phase-gated computation:** execute only when clocks align at specific points
- **Phase-weighted computation:** use phase as arithmetic coefficients
- **Phase-encoded computation:** multiplex N values in one register via time-division

I also sketched an ISA extension (RDPHI instruction) — a flip-flop that samples one clock domain on the rising edge of another. ~200 gates of silicon, projected 100-300x throughput improvement over the software method.

And there's an encryption scheme where the key is the temporal relationship between clock frequencies at a specific instant. A 1 microsecond timing error produces completely different output.

### What I'm looking for

- Feedback from people who work with clock domains, FPGA design, or CPU architecture
- Anyone who wants to try it on different hardware (ARM, x86 Linux, RISC-V)
- People who can poke holes in the statistical analysis
- Ideas for applications I haven't thought of

Code: https://github.com/Alemusica/triphase-computation

*Patent pending (Italian Patent Office, Feb 2026). Source available under BSL 1.1 — free for research, testing, personal projects. Converts to Apache 2.0 in 2030.*

---

## Suggested comments strategy

Reply to early comments quickly. Be ready for:
- "This is just jitterentropy" → No: jitterentropy treats timing as noise to collect (~1 bit/sample). This formalizes the phase *relationship* as a computational resource (~4 phits/sample), and uses it for routing/crypto, not just entropy.
- "NIST-inspired isn't NIST-certified" → Correct. This is a proof of concept. Full SP 800-22 / 800-90B testing would be the next step for production crypto use.
- "Lock-free routing sounds like just random selection" → It is, but the randomness comes from physics (clock phase) instead of software state. Zero contention by construction. No shared memory, no atomics, no mutexes.
- "The encryption scheme sounds impractical" → It's deliberately speculative. The core contribution is phit extraction + routing. Crypto is a research direction.
