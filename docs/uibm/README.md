# UIBM Patent Filing — Triphase Computation / Phit

## Status

- [ ] Source documents prepared (descrizione, rivendicazioni, riassunto)
- [ ] Clean text conversion (convert_clean.py)
- [ ] PDF generation (make_pdfs.py)
- [ ] Digital signature (CIE, PAdES format)
- [ ] PDF/A conversion
- [ ] Upload to UIBM portal
- [ ] F24 payment (EUR 50, codice tributo 6936)

## Filing Details

| Field | Value |
|-------|-------|
| Type | Brevetto per invenzione industriale (Ordinaria) |
| Portal | https://servizionline.uibm.gov.it |
| Authentication | SPID or CIE |
| Cost | EUR 50 (10 claims, no overage) |
| Claims | 10 (3 independent + 7 dependent) |
| Language | EN (title in IT required) |

## Italian Title
"Metodo e sistema per l'estrazione e l'utilizzo di bit di fase computazionali (phit) dalle relazioni tra domini di clock asincroni nei processori digitali"

## IPC Classification
- G06F 1/04 — Clock signal generation/distribution
- G06F 7/58 — Random/pseudo-random number generation
- H04L 9/08 — Key distribution using clock signals
- G06F 9/46 — Multiprocessing scheduling

## Claim Structure

### Independent (3)
1. **Phit extraction method** — Core invention: workload + timer + hash → phit
2. **Lock-free task routing** — Phase-based routing without shared state
3. **RDPHI processor instruction** — Hardware phase readout for computation

### Dependent (7)
4. Compound sampling N=2 optimal (→ Claim 1)
5. Timer LSB uniformity validation (→ Claim 1)
6. PRNG with entropy pool and forward security (→ Claim 1)
7. Phase-gated encryption from phase vector (→ Claim 1)
8. Phase-locked access windows (→ Claim 7)
9. PHIGATE/PHIWEIGHT companion instructions (→ Claim 3)
10. Portable software library (→ Claim 1)

## Prior Art Cited
- Jitterentropy (Muller) — CPU jitter as entropy, ~1 bit/sample
- US10498528B2 (Clock Machines, Kwok, 2019) — single clock, discrete time states
- RISC-V entropy source (Neumann et al., 2022) — ISA entropy-only interface
- Intel RDRAND/RDSEED — dedicated hardware RNG circuit
- Ring oscillator TRNGs — hardware phase jitter on FPGAs

## Related Filing
- Monade Geodensa: IT 102026000002875 (filed 2026-02-05)
- Same inventor, same filing procedure

## Processing Workflow
```
source/*.md → convert_clean.py → *_clean.txt → make_pdfs.py → *_FINAL.pdf → CIE sign → signed/
```
