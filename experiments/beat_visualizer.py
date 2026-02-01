"""
Beat Pattern Visualizer
========================

Visualizza i pattern di battimento tra i clock nel terminale.
Mostra dove avvengono i momenti di sincronia e come l'informazione
di fase evolve nel tempo.

Author: Alessio Cazzaniga
"""

import sys
import os
import math

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))
from triphase_sim import simple_system, m1_max_system


def visualize_beat(f_alpha: float = 5.0, f_beta: float = 3.0,
                   duration: float = 2.0, resolution: int = 80):
    """Visualizza il battimento Alpha-Beta nel tempo."""

    sys_clk = simple_system(f_alpha, f_beta, 1.0)
    beat_f = sys_clk.beat_frequency_ab()

    print(f"\n  Alpha: {f_alpha} Hz  |  Beta: {f_beta} Hz  |  Beat: {beat_f} Hz")
    print(f"  Ratio: {f_alpha/f_beta:.4f}:1")
    print(f"  Duration: {duration}s")
    print()

    width = resolution
    steps = 200
    dt = duration / steps

    for i in range(steps):
        t = i * dt
        phi = sys_clk.phase_ab(t)  # [-0.5, 0.5)

        # Map to display position [0, width)
        pos = int((phi + 0.5) * width) % width
        center = width // 2

        line = list(' ' * width)
        line[center] = '|'  # center marker

        is_sync = abs(phi) < 0.05
        char = '*' if is_sync else '.'
        line[pos] = char

        time_str = f"{t:.3f}s"
        phi_str = f"Φ={phi:+.3f}"
        sync_str = " <<<SYNC" if is_sync else ""

        print(f"  {time_str} [{' '.join(line[:40])}] {phi_str}{sync_str}")


def visualize_torus(f1: float = 5.0, f2: float = 3.0, f3: float = 2.0,
                    duration: float = 3.0):
    """
    Proiezione 2D della traiettoria sul toro 3D.
    Mostra Φ_AB vs Φ_AO come scatter ASCII.
    """
    sys_clk = simple_system(f1, f2, f3)

    print(f"\n  Torus projection: Φ_AB vs Φ_AO")
    print(f"  Clocks: {f1}, {f2}, {f3} Hz")
    print()

    grid_size = 40
    grid = [[' ' for _ in range(grid_size)] for _ in range(grid_size)]

    steps = 500
    dt = duration / steps
    sync_count = 0

    for i in range(steps):
        t = i * dt
        phi_ab = sys_clk.phase_ab(t)  # [-0.5, 0.5)
        phi_ao = sys_clk.phase_ao(t)  # [-0.5, 0.5)

        x = int((phi_ab + 0.5) * (grid_size - 1))
        y = int((phi_ao + 0.5) * (grid_size - 1))
        x = max(0, min(grid_size - 1, x))
        y = max(0, min(grid_size - 1, y))

        is_sync = abs(phi_ab) < 0.05 and abs(phi_ao) < 0.05
        if is_sync:
            grid[y][x] = '*'
            sync_count += 1
        elif grid[y][x] == ' ':
            grid[y][x] = '.'

    # Print grid
    print(f"  Φ_AO")
    print(f"  +0.5 {'─' * grid_size}┐")
    for row in range(grid_size):
        label = f"       " if row != grid_size // 2 else f"   0.0 "
        print(f"  {label}│{''.join(grid[row])}│")
    print(f"  -0.5 {'─' * grid_size}┘")
    print(f"       -0.5{' ' * (grid_size - 8)}+0.5")
    print(f"                    Φ_AB")
    print(f"\n  Sync points found: {sync_count}/{steps}")

    # Density analysis
    filled = sum(1 for row in grid for c in row if c != ' ')
    coverage = filled / (grid_size * grid_size)
    print(f"  Phase space coverage: {coverage*100:.1f}%")
    print(f"  (Ergodic trajectory fills more space over time)")


def visualize_information_density():
    """
    Mostra come la densità informativa cresce con il numero di slot.
    """
    print(f"\n  === Information Density Scaling ===\n")
    print(f"  {'Slots':>6} | {'Bits/reg':>8} | {'32-reg values':>14} | {'Gain':>6}")
    print(f"  {'------':>6}-+-{'--------':>8}-+-{'-'*14}-+-{'------':>6}")

    for slots in [1, 2, 4, 8, 16, 32, 64]:
        bits = math.log2(slots) if slots > 1 else 0
        total = 32 * slots
        gain = f"{slots}x"
        bar = '#' * min(slots, 40)
        print(f"  {slots:6d} | {bits:8.1f} | {total:14d} | {gain:>6} {bar}")

    print()
    print("  Theoretical limit: bounded by phase resolution")
    print("  On M1 Max with 24MHz timer vs 3.2GHz CPU:")
    print(f"    Max distinguishable phases ≈ {3228/24:.0f}")
    print(f"    Max extra bits ≈ {math.log2(3228/24):.1f}")


if __name__ == "__main__":
    print("╔══════════════════════════════════════════════════════════╗")
    print("║  TRIPHASE: Beat Pattern Visualizer                      ║")
    print("╚══════════════════════════════════════════════════════════╝")

    # Simple case: 5:3 ratio
    visualize_beat(5.0, 3.0, duration=1.0, resolution=40)

    # Golden ratio
    print("\n" + "="*60)
    print("  Golden Ratio Clock Pair")
    print("="*60)
    visualize_beat(1.618, 1.0, duration=3.0, resolution=40)

    # Torus projection
    visualize_torus(5.0, 3.0, 2.0)

    # Density scaling
    visualize_information_density()
