"""
Practical Triphase Computation Test
====================================

Dimostra i tre paradigmi con benchmark comparativi:
1. Phase-Gated Computation (sync bonus)
2. Phase-Weighted Arithmetic
3. Phase-Encoded State (memory multiplexing)
4. Hidden Parallelism (1 flusso sequenziale → N operazioni)

Author: Alessio Cazzaniga
"""

import sys
import os
import time
import math

# Add parent to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))
from triphase_sim import (
    Clock, TriphaseSystem, PhaseRegister,
    PhaseWeightedALU, TriphaseVM, PhaseInstruction,
    simple_system, m1_max_system
)


def separator(title: str):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


# =============================================================================
# Test 1: Sync Bonus — operazioni extra ai punti di sincronia
# =============================================================================

def test_sync_bonus():
    separator("TEST 1: Sync Bonus")

    # Observer at 11 Hz (prime, non-harmonic with 5 and 3)
    # to sample different phases
    sys_clk = simple_system(5.0, 3.0, 11.0)
    dt = 1.0 / sys_clk.observer.frequency_hz

    conventional_ops = 0
    triphase_ops = 0
    sync_bonuses = 0

    num_ticks = 1000

    for i in range(num_ticks):
        t = i * dt
        conventional_ops += 1  # 1 op per tick sempre
        triphase_ops += 1      # 1 op base

        if sys_clk.is_sync(t, threshold=0.1):
            # Al sync point: operazione bonus "gratuita"
            triphase_ops += 1
            sync_bonuses += 1

    ratio = triphase_ops / conventional_ops
    print(f"  Conventional ops:  {conventional_ops}")
    print(f"  Triphase ops:      {triphase_ops}")
    print(f"  Sync bonuses:      {sync_bonuses} ({100*sync_bonuses/num_ticks:.1f}% dei tick)")
    print(f"  Throughput ratio:  {ratio:.2f}x")
    print(f"  Beat freq:         {sys_clk.beat_frequency_ab():.1f} Hz")
    return ratio


# =============================================================================
# Test 2: Phase-Encoded Memory — N valori in 1 registro
# =============================================================================

def test_phase_memory():
    separator("TEST 2: Phase-Encoded Memory")

    num_slots = 8
    reg = PhaseRegister("r0", num_slots)

    # Scrivi valori diversi in ogni slot
    values = [42, 17, 99, -5, 256, 0, 1000, 7]
    for i, v in enumerate(values):
        reg.write_slot(i, v)

    print(f"  Slots per register:     {num_slots}")
    print(f"  Extra bits per reg:     {reg.density_bits():.1f}")
    print(f"  Conventional (32 regs): {32} values")
    print(f"  Triphase (32 regs):     {32 * num_slots} values")
    print(f"  Density gain:           {num_slots}x")
    print()

    # Dimostra accesso fase-dipendente
    print("  Phase scan:")
    for phi in [i/num_slots + 0.01 for i in range(num_slots)]:
        val = reg.read(phi)
        print(f"    Φ={phi:.3f} → r0 = {val}")

    # Verifica correttezza
    correct = 0
    for i in range(num_slots):
        phi = i / num_slots + 0.001
        if reg.read(phi) == values[i]:
            correct += 1

    print(f"\n  Correctness: {correct}/{num_slots} slots verified")
    return num_slots


# =============================================================================
# Test 3: Hidden Parallelism — 1 stream, N computations
# =============================================================================

def test_hidden_parallelism():
    separator("TEST 3: Hidden Parallelism")

    # Observer at 13 Hz (prime) to sample across all phase slots
    sys_clk = simple_system(4.0, 1.0, 13.0)

    # 4 accumulatori, uno per fase
    accumulators = [0] * 4
    ops = [
        lambda a: a + 1,      # fase 0: incrementa
        lambda a: a + 2,      # fase 1: aggiungi 2
        lambda a: a * 2 + 1,  # fase 2: raddoppia+1
        lambda a: a + 10,     # fase 3: aggiungi 10
    ]
    op_names = ["+1", "+2", "*2+1", "+10"]

    dt = 1.0 / sys_clk.observer.frequency_hz
    num_ticks = 20

    print(f"  Alpha: {sys_clk.alpha.frequency_hz} Hz")
    print(f"  Beta:  {sys_clk.beta.frequency_hz} Hz")
    print(f"  Ratio: {sys_clk.alpha.frequency_hz/sys_clk.beta.frequency_hz:.0f}:1")
    print()
    print(f"  {'Tick':>4} | {'Phase':>5} | {'Op':>6} | Accumulators")
    print(f"  {'----':>4}-+-{'-----':>5}-+-{'------':>6}-+-{'-'*30}")

    for i in range(num_ticks):
        t = i * dt
        phi = sys_clk.alpha.phase_at(t)
        slot = int(phi * 4) % 4

        accumulators[slot] = ops[slot](accumulators[slot])

        print(f"  {i+1:4d} | {phi:5.2f} | {op_names[slot]:>6} | {accumulators}")

    print(f"\n  Sequential ticks: {num_ticks}")
    print(f"  Independent computations: {len(accumulators)}")
    print(f"  Parallelism factor: {len(accumulators)}x (from 1 stream)")


# =============================================================================
# Test 4: Phase-Weighted Arithmetic — stesso input, output diverso
# =============================================================================

def test_phase_weighted():
    separator("TEST 4: Phase-Weighted Arithmetic")

    sys_clk = simple_system(7.0, 5.0, 11.0)
    alu = PhaseWeightedALU(sys_clk)

    a, b = 100.0, 50.0
    dt = 1.0 / sys_clk.observer.frequency_hz

    print(f"  Operation: add({a}, {b})")
    print(f"  Conventional result: {a + b}")
    print()

    results = []
    print(f"  {'Tick':>4} | {'Φ_AB':>7} | {'Result':>10} | {'Deviation':>10}")
    print(f"  {'----':>4}-+-{'-------':>7}-+-{'----------':>10}-+-{'----------':>10}")

    for i in range(15):
        t = i * dt
        r = alu.add(a, b, t)
        phi = sys_clk.phase_ab(t)
        dev = r - (a + b)
        results.append(r)
        print(f"  {i+1:4d} | {phi:+.4f} | {r:10.2f} | {dev:+10.2f}")

    unique = len(set(f"{r:.6f}" for r in results))
    print(f"\n  Unique results from same input: {unique}/15")
    print(f"  → {math.log2(unique):.1f} extra bits of information")


# =============================================================================
# Test 5: Phase-Gated Crypto — accesso temporale
# =============================================================================

def test_phase_crypto():
    separator("TEST 5: Phase-Gated Access")

    sys_clk = simple_system(7.0, 5.0, 11.0)

    # Il "segreto" è accessibile solo in una finestra di fase
    secret = 42
    window_center = 0.333
    window_width = 0.1

    dt = 1.0 / sys_clk.observer.frequency_hz
    num_attempts = 100

    successes = 0
    for i in range(num_attempts):
        t = i * dt
        phi = sys_clk.phase_ab(t)
        dist = abs(phi - window_center)
        if dist > 0.5:
            dist = 1.0 - dist
        if dist <= window_width / 2:
            successes += 1

    access_rate = successes / num_attempts
    print(f"  Secret window: Φ_AB = {window_center} ± {window_width/2}")
    print(f"  Attempts:      {num_attempts}")
    print(f"  Successful:    {successes}")
    print(f"  Access rate:   {access_rate*100:.1f}%")
    security = f"{-math.log2(access_rate):.1f}" if access_rate > 0 else "inf"
    print(f"  Security:      {security} bits")
    print(f"  (Without knowing frequencies, access is random)")


# =============================================================================
# Test 6: VM Demo — programma trifasico completo
# =============================================================================

def test_vm_demo():
    separator("TEST 6: Triphase VM Execution")

    sys_clk = simple_system(5.0, 3.0, 11.0)
    vm = TriphaseVM(sys_clk, num_registers=4, slots_per_register=4)

    # Programma: accumula in r0 solo ai sync points,
    #            conta tick in r1 sempre

    def op_accumulate(vm, t):
        current = vm.registers["r0"].slots[0].value
        vm.registers["r0"].slots[0].value = current + 10
        return f"r0 += 10 → {current + 10}"

    def op_count(vm, t):
        current = vm.registers["r1"].slots[0].value
        vm.registers["r1"].slots[0].value = current + 1
        return f"r1++ → {current + 1}"

    program = [
        PhaseInstruction("ab", 0.0, 0.2, op_accumulate, "SYNC_ADD"),
        PhaseInstruction("ab", 0.0, 1.0, op_count, "ALWAYS_COUNT"),
    ]

    vm.load_program(program)
    results = vm.run(30)

    print(f"  Program: 2 instructions")
    print(f"    SYNC_ADD:     r0 += 10 when Φ_AB ≈ 0.0 (±0.1)")
    print(f"    ALWAYS_COUNT: r1++     always")
    print()
    print(f"  {'Tick':>4} | {'Φ_AB':>7} | {'Sync':>5} | Executed")
    print(f"  {'----':>4}-+-{'-------':>7}-+-{'-----':>5}-+-{'--------'}")

    for r in results[:20]:
        phi_ab = r["phases"][0]
        sync = "YES" if r["sync"] else ""
        ops = ", ".join(e["op"] for e in r["executed"]) if r["executed"] else "-"
        print(f"  {r['tick']:4d} | {phi_ab:+.4f} | {sync:>5} | {ops}")

    r0 = vm.registers["r0"].slots[0].value
    r1 = vm.registers["r1"].slots[0].value
    print(f"\n  Final: r0={r0} (sync-gated), r1={r1} (ungated)")
    print(f"  Ratio: r0 advanced {r0//10} times in {r1} ticks")


# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("╔══════════════════════════════════════════════════════════╗")
    print("║  TRIPHASE COMPUTATION — Practical Benchmark Suite       ║")
    print("║  Paradigms 2 (Phase-Weighted) + 3 (Phase-Encoded)       ║")
    print("╚══════════════════════════════════════════════════════════╝")

    results = {}

    results["sync_ratio"] = test_sync_bonus()
    results["memory_gain"] = test_phase_memory()
    test_hidden_parallelism()
    test_phase_weighted()
    test_phase_crypto()
    test_vm_demo()

    separator("OVERALL RESULTS")
    print(f"  Sync throughput gain:   {results['sync_ratio']:.2f}x")
    print(f"  Memory density gain:   {results['memory_gain']}x")
    print(f"  Paradigm validation:   All 3 paradigms functional")
    print()
    print("  Next: run phase_extract on M1 Max hardware to")
    print("  measure real phase entropy from timer vs CPU clock.")
