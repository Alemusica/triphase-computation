# Reddit Post Draft — Triphase Computation

## Target subreddits (start with 1, expand if traction)
- r/programming (1.1M+ members, general audience) — **start here**
- r/compsci (500K+, theoretical angle) — good second choice
- r/netsec — skip for now (BSL + patent will trigger hostility in security community)
- r/hardware — skip for now (need more benchmark data first)
- **Consider also:** Hacker News (news.ycombinator.com) — technical audience, less license hostility

## Posting strategy
- **When:** Tuesday or Wednesday, ~14:00 CET (morning US East Coast)
- **NOT:** Friday/weekend (low visibility)
- **Before posting:** Check your Reddit account karma — low-karma accounts get filtered
- **First hour:** Be online to reply to early comments immediately

---

## Title options (pick one)

1. "Your CPU has been throwing away free entropy — I found how to catch it"
2. "I extracted usable randomness from the phase relationship between async clock domains — no hardware mod needed"
3. "Lock-free task routing without shared counters: using clock phase relationships instead"

---

## Post body (for r/programming, adapt for others)

---

**TL;DR:** Every modern CPU has multiple asynchronous clock domains. The phase relationship between them changes continuously and contains extractable information. I wrote a header-only C library that harvests this "phase entropy" on commodity hardware — no special instructions needed. It passes NIST SP 800-22-inspired statistical tests, does lock-free task routing without shared counters or mutexes, and I think it could be a useful primitive. Source available (BSL 1.1 — free for non-production use, Apache 2.0 in 2030).

---

### The insight

Your CPU has at least 3 independent clocks: performance cores (~3.2 GHz), efficiency cores (~2 GHz), and a system timer (24 MHz). These are asynchronous — their phase relationship shifts continuously.

Current engineering treats this as noise. Clock domain crossing circuits, synchronization barriers, metastability guards — all designed to *eliminate* phase information.

I asked: what if it's not noise? What if it's a resource?

### What I built

**libphit.h** — a header-only C library (~390 lines, no dependencies beyond libc) that extracts "phits" (phase-bits) from existing hardware:

```c
#define LIBPHIT_IMPLEMENTATION
#include "libphit.h"

// Extract a phase sample — no special hardware needed
uint32_t sample = phit_sample_compound(2);  // ~4 phits

// Use it for RNG
phit_prng_t rng;
phit_prng_init(&rng);
uint64_t random_val = phit_prng_u64(&rng);

// Or for lock-free routing — no shared counters, no mutexes
int worker = phit_route(num_workers);
```

### How it works

1. Execute a calibrated workload (tiny LCG loop, ~800ns)
2. Read the system timer immediately after
3. The timer's 2 LSBs show strong uniformity (Chi-squared = 0.39 across 4 bins, threshold 7.81 at p=0.05) due to the async clock relationship
4. Combine timer LSBs + workload timing delta + hash → phit sample
5. Compound sampling (N=2 reads) gives ~4.06 phits per sample

The math: when clock frequencies are incommensurable (irrational ratio), Weyl's Equidistribution Theorem guarantees uniform phase distribution. The Three-Distance Theorem constrains the sampling structure. The extractable entropy comes from hardware jitter amplified by phase structure.

### Results on Apple M1 Max

**PRNG quality (100K samples, NIST SP 800-22-inspired):**
- Monobit test: PASS
- Runs test: PASS
- Byte distribution: Chi-squared = 221.7 (< 310 critical)
- Per-bit entropy: 64.0/64.0 bits (100%)
- Throughput: 181 Mbit/s

**Lock-free scheduler (100K tasks, 8 workers):**
- Uniformity: Chi-squared = 7.8 (threshold: 14.07)
- Max load imbalance: 2.5%
- Shared counters/mutexes/atomics: **none**
- Lock contention: **zero**
- Throughput: 28 Mroute/s

### Where this could go

I think phase relationships between clocks could be a useful computational primitive. The library explores three directions:

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
- "NIST-inspired isn't NIST-certified" → Correct. This is a proof of concept. Full SP 800-22 / 800-90B testing would be the next step for production crypto use. The current tests are directional, not certification-grade.
- "Lock-free routing sounds like just random selection" → It is, but the randomness comes from physics (clock phase) instead of software state. No shared counters, no atomics, no mutexes needed for coordination.
- "The encryption scheme sounds impractical" → It's deliberately speculative. The core contribution is phit extraction + routing. Crypto is a research direction.
- "Why -O0? That's cheating" → The workload creates a timing delta that the phase measurement depends on. -O0 ensures the compiler doesn't eliminate it. A hardware RDPHI instruction wouldn't need this — that's why the ISA extension matters.
- "Why not just use /dev/urandom?" → You can, and should, for cryptographic randomness. This is about a different primitive: using phase relationships for lock-free coordination, not replacing OS entropy sources.
- "BSL isn't open source" → Correct — it's source-available. Free for research, evaluation, personal projects. Production use needs a license. Converts to Apache 2.0 in 2030. I'm a solo developer protecting my work while sharing the ideas.
- "Patent pending on publicly funded research?" → Self-funded, solo work. The patent protects the method; the code is freely available for non-production use. Feedback and collaboration are what I'm here for.
- "What about VMs / containers / cloud?" → Timer resolution varies by environment. Bare metal gives best results. Containers with host clock access work. Some VMs may virtualize timers — an open research question.
- "Sample size is small for statistical claims" → Fair point. 100K samples is enough for the NIST-inspired tests used, but I'd welcome someone running this through the full NIST SP 800-22 battery or TestU01.
