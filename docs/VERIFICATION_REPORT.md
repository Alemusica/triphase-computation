# Verification Report — Triphase Computation

*Analysis date: 2026-02-12*
*Purpose: Identify test gaps and statistical rigor issues*

---

## 1. Current Test Coverage

### What test_libphit.c Actually Verifies
- Hash determinism: phit_hash32(42) == phit_hash32(42) — trivially correct
- PRNG non-degeneracy: two consecutive u64 calls differ — extremely weak
- Monobit (1000 values, 0.45-0.55 threshold) — coarse sanity check
- Routing uniformity (10K routes, 8 buckets, Chi²<30) — very generous threshold
- Timer advancing: t2 > t1 after workload

### What is NOT Tested

| Missing Test | Severity | Why It Matters |
|---|---|---|
| Autocorrelation at lags 1-100 | **CRITICAL** | Cannot validate "decorrelated" claim |
| Serial independence between samples | **CRITICAL** | Undermines all entropy-rate claims |
| Full NIST SP 800-22 (15 tests) | **HIGH** | Only 4 of 15 implemented |
| Cross-platform validation | **HIGH** | Portability claim unvalidated |
| Stress test under CPU load | **HIGH** | No scheduling interference test |
| Long-duration stability (>1M samples) | **MEDIUM** | 100K may mask drift |
| Distribution stationarity | **HIGH** | Code acknowledges non-stationarity but never tests |
| Multiple runs with aggregate pass rates | **MEDIUM** | Single-run results not reproducible |
| Edge cases: phit_route(0), prng_range(0) | **LOW** | Division by zero |

---

## 2. Statistical Rigor

### Chi-Squared Critical Values: INCONSISTENT

| Location | Value Used | Correct Value (df=7, p=0.05) | Assessment |
|---|---|---|---|
| test_libphit.c self-test | 30.0 | 14.07 | TOO PERMISSIVE — never fails |
| phit_scheduler.c | 14.07 | 14.07 | Correct |
| phi_exploit.c | 14.07 | 14.07 | Correct |
| phi_adaptive.c | df + 2√(2df) | ~14.48 | Close enough |
| phi_uniform.c | df + 2√(df) | ~12.29 | TOO STRICT — false rejections |
| phi_adaptive.c (×3) | chi2_crit * 3 | ~42 | EXTREMELY PERMISSIVE |

**Fix:** Standardize with Wilson-Hilferty approximation:
```
chi2_crit(df, alpha) = df * (1 - 2/(9*df) + z_alpha * sqrt(2/(9*df)))^3
```
where z_alpha = 1.645 for alpha=0.05.

### NIST SP 800-22: OVERSTATED

Currently implemented: 4 of 15 tests
- Monobit ✓
- Runs ✓
- Byte distribution (frequency-within-block) ✓
- Per-bit entropy (custom, not NIST) ✓

NOT implemented:
- Discrete Fourier Transform (spectral)
- Approximate Entropy
- Serial Test
- Cumulative Sums
- Random Excursions / Variant
- Overlapping Template Matching
- Linear Complexity
- Maurer's Universal Statistical Test
- Binary Matrix Rank
- Longest Run of Ones
- Non-overlapping Template Matching

**Fix:** Either run full NIST STS or change language to "subset of NIST-inspired tests."

### 1.96 Phit/Read Measurement

**Issues:**
1. Uses Shannon entropy, not min-entropy (H_inf ≤ H)
2. Assumes IID samples — not validated (no autocorrelation test)
3. Conflates two entropy sources: timer LSBs (~2 bits, provably uniform) + workload delta (~0-1 bit)
4. Histogram binning at 1ns may overcount due to quantization noise

**Verdict:** 1.96 is plausible Shannon entropy but likely overestimates extractable randomness. Min-entropy may be closer to 1.0-1.5 phits per read.

---

## 3. Portability Risks

### Intel/AMD
- TSC has sub-nanosecond resolution (vs ARM's 42ns ticks)
- Delta distribution will be completely different
- Hardcoded 42ns tick in phit__quantize() will produce wrong quantization
- CDF calibration approach would adapt, but library doesn't use it

### Linux
- clock_gettime(CLOCK_MONOTONIC_RAW) can source from TSC, HPET, or acpi_pm
- HPET: ~100ns resolution → may work
- acpi_pm: ~1μs resolution → approach collapses
- No runtime detection of clock source

### Heavy CPU Load
- Context switches insert microsecond-scale timing deltas
- Delta distribution becomes multimodal with long tail
- Entropy may increase (more distinct values) but non-stationary
- NOT TESTED AT ALL

### CPU Frequency Scaling
- Code documents this as "non-stationarity" (phi_adaptive.c)
- Compound sampling claimed robust — NOT TESTED under deliberate frequency changes
- Battery vs plugged in may produce different behavior

### Virtualized Environments
- Timer reads may be intercepted by hypervisor
- kvmclock destroys the asynchronous relationship
- NOT ADDRESSED in specification

---

## 4. Specific Tests to Add

### Priority: CRITICAL

**test_autocorrelation:**
```c
// Generate 100K phit samples
// Compute Pearson r at lags 1, 2, 5, 10, 50, 100
// Assert |r| < 3/sqrt(N) for all lags
```

**test_serial_independence:**
```c
// Generate pairs (phit_sample(), phit_sample())
// Build 2D histogram of (sample_i % K, sample_{i+1} % K)
// Chi-squared on K*K contingency table, df = (K-1)^2
```

### Priority: HIGH

**test_cross_platform_timer:**
```c
// Detect actual timer resolution at runtime
// Measure minimum non-zero delta
// Adapt quantization accordingly
```

**test_uniformity_under_load:**
```c
// Spawn N background threads doing busy work
// Measure phit routing uniformity during load
// Assert Chi-squared below critical value
```

**test_nist_full:**
```bash
# Export 1M+ bytes to binary file
# Run against NIST SP 800-22 sts tool
# Run against dieharder
# Run against TestU01 BigCrush
```

### Priority: MEDIUM

**test_long_duration:**
```c
// 10M+ samples divided into 100 blocks of 100K
// Chi-squared per block
// Assert no more than 5% of blocks fail at p=0.05
```

**test_frequency_scaling:**
```c
// Alternate heavy/light workloads to trigger DVFS
// Measure phit distribution before/after
// Assert routing remains approximately uniform
```

**test_prng_range_bias:**
```c
// Test with max = 3, 5, 7, 100
// Verify uniformity (modulo bias check)
```

---

## 5. Reproducibility Assessment

### Can reproduce on same hardware (M1 Max): PARTIALLY
- Code compiles with gcc -O0
- Results will be similar but not identical (stochastic)
- Pass/fail thresholds generous enough to pass consistently

### Cannot reproduce:
1. **Optimization level dependency** — -O0 required but not enforced in code
2. **Platform dependency** — 42ns tick, 1.96 phit figure are M1 Max specific
3. **No automated test runner** — no Makefile, no CI, no run_all_tests.sh
4. **No reference outputs** — no baselines to compare against
5. **Environment not captured** — no record of kernel version, CPU governor, system load, power source, thermal state

### Missing for reproducibility
- Makefile or build system
- Test harness with exit codes (0 = pass, 1 = fail)
- Configuration documentation (CPU governor, Turbo state, system idle?)
- Seed documentation (effect of varying 0xCAFEBABE workload seed?)
- Statistical protocol (how many independent runs? what is run-to-run variance?)

---

## 6. Positive Findings

1. Entropy pool design (SplitMix64 + cross-lane rotation) is sound
2. Forward-security mechanism in pool extraction is well-designed
3. Compound sampling genuinely increases effective entropy
4. phit_crypto.c honestly disclaims "NOT cryptographically secure"
5. CDF calibration in phi_uniform.c is the most rigorous routing method
6. Timer LSB uniformity (Chi²=0.39) is well-measured and convincing

---

## 7. Summary: Fix Priority

| Priority | What | Effort |
|---|---|---|
| **1** | Autocorrelation + serial independence tests | 1 day |
| **2** | Standardize Chi-squared critical values | 2 hours |
| **3** | Fix NIST language OR run full NIST STS | 1 day (language) or 1 week (full suite) |
| **4** | Runtime timer resolution detection | 4 hours |
| **5** | Makefile + test harness | 4 hours |
| **6** | Cross-platform test (Linux x86) | 1-2 days |
| **7** | Min-entropy analysis (NIST 800-90B) | 2-3 days |
| **8** | Fixed-frequency experiment | 1 day |
| **9** | Stress test under load | 4 hours |
| **10** | Fix phit_route(0) + prng_range modulo bias | 1 hour |
