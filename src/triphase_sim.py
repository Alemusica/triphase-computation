"""
Triphase Computation Simulator
==============================

Modello di esecuzione trifasico: tre clock asincroni dove
l'informazione di fase relativa diventa risorsa computazionale.

Σ(t) = (S(t), Φ(t))
dove S = stato spaziale, Φ = vettore di fasi

Author: Alessio Cazzaniga
"""

import math
from dataclasses import dataclass, field
from typing import Callable, Optional


# =============================================================================
# Core: Clock Domains
# =============================================================================

@dataclass
class Clock:
    """Un dominio di clock con frequenza fissa."""
    name: str
    frequency_hz: float
    _ticks: int = 0

    @property
    def period(self) -> float:
        return 1.0 / self.frequency_hz

    def phase_at(self, t: float) -> float:
        """Fase normalizzata [0, 1) al tempo t."""
        return (t * self.frequency_hz) % 1.0

    def tick(self) -> int:
        self._ticks += 1
        return self._ticks


@dataclass
class TriphaseSystem:
    """
    Sistema a tre clock: Alpha, Beta, Observer.

    Alpha e Beta generano il battimento.
    Observer campiona e calcola.
    """
    alpha: Clock
    beta: Clock
    observer: Clock

    def phase_ab(self, t: float) -> float:
        """Fase relativa Alpha-Beta, normalizzata [-0.5, 0.5)."""
        delta = self.alpha.phase_at(t) - self.beta.phase_at(t)
        # Wrap to [-0.5, 0.5)
        return delta - round(delta)

    def phase_ao(self, t: float) -> float:
        delta = self.alpha.phase_at(t) - self.observer.phase_at(t)
        return delta - round(delta)

    def phase_bo(self, t: float) -> float:
        delta = self.beta.phase_at(t) - self.observer.phase_at(t)
        return delta - round(delta)

    def phase_vector(self, t: float) -> tuple:
        """Vettore completo di fasi (Φ_AB, Φ_AO, Φ_BO)."""
        return (self.phase_ab(t), self.phase_ao(t), self.phase_bo(t))

    def beat_frequency_ab(self) -> float:
        """Frequenza di battimento Alpha-Beta."""
        return abs(self.alpha.frequency_hz - self.beta.frequency_hz)

    def is_sync(self, t: float, threshold: float = 0.05) -> bool:
        """True se Alpha e Beta sono quasi in fase."""
        return abs(self.phase_ab(t)) < threshold

    def sync_points(self, t_start: float, t_end: float,
                    threshold: float = 0.05, resolution: int = 10000) -> list:
        """Trova i punti di sincronia in un intervallo."""
        dt = (t_end - t_start) / resolution
        points = []
        for i in range(resolution):
            t = t_start + i * dt
            if self.is_sync(t, threshold):
                points.append(t)
        return points


# =============================================================================
# Execution Model: Phase-Gated Instructions
# =============================================================================

@dataclass
class PhaseInstruction:
    """
    Istruzione trifasica: WHEN-WHAT-WHERE

    WHEN: condizione sulla fase (quale coppia, finestra)
    WHAT: operazione da eseguire
    WHERE: destinazione + phase_tag
    """
    phase_pair: str            # "ab", "ao", "bo"
    window_center: float       # centro della finestra di fase
    window_width: float        # ampiezza della finestra
    operation: Callable        # funzione da eseguire
    op_name: str = "nop"       # nome leggibile

    def can_execute(self, system: TriphaseSystem, t: float) -> bool:
        """L'istruzione può eseguire in questo istante?"""
        if self.phase_pair == "ab":
            phi = system.phase_ab(t)
        elif self.phase_pair == "ao":
            phi = system.phase_ao(t)
        elif self.phase_pair == "bo":
            phi = system.phase_bo(t)
        else:
            return False

        dist = abs(phi - self.window_center)
        if dist > 0.5:
            dist = 1.0 - dist
        return dist <= self.window_width / 2


# =============================================================================
# Phase-Encoded Memory (Paradigma 3)
# =============================================================================

@dataclass
class PhaseSlot:
    """Un valore valido solo in una finestra di fase."""
    value: object
    phase_start: float
    phase_end: float

    def is_valid(self, phi: float) -> bool:
        if self.phase_start <= self.phase_end:
            return self.phase_start <= phi < self.phase_end
        else:  # wraps around
            return phi >= self.phase_start or phi < self.phase_end


class PhaseRegister:
    """
    Registro a fase: contiene N valori multiplexati nel tempo.

    Un singolo registro fisico che contiene K valori diversi,
    ciascuno accessibile solo nella propria finestra di fase.

    Densità informativa: log2(K) bit aggiuntivi per registro.
    """
    def __init__(self, name: str, num_slots: int = 4):
        self.name = name
        self.slots: list[PhaseSlot] = []
        self.num_slots = num_slots
        # Crea slot uniformi
        slot_width = 1.0 / num_slots
        for i in range(num_slots):
            self.slots.append(PhaseSlot(
                value=0,
                phase_start=i * slot_width,
                phase_end=(i + 1) * slot_width
            ))

    def read(self, phi: float) -> object:
        """Leggi il valore corrispondente alla fase corrente."""
        phi_norm = phi % 1.0
        for slot in self.slots:
            if slot.is_valid(phi_norm):
                return slot.value
        return None

    def write(self, phi: float, value: object):
        """Scrivi nel slot corrispondente alla fase corrente."""
        phi_norm = phi % 1.0
        for slot in self.slots:
            if slot.is_valid(phi_norm):
                slot.value = value
                return True
        return False

    def write_slot(self, index: int, value: object):
        """Scrivi direttamente in uno slot per indice."""
        if 0 <= index < len(self.slots):
            self.slots[index].value = value

    def density_bits(self) -> float:
        """Bit aggiuntivi di densità informativa."""
        return math.log2(self.num_slots)

    def dump(self) -> dict:
        return {
            f"slot_{i} [{s.phase_start:.2f}-{s.phase_end:.2f})": s.value
            for i, s in enumerate(self.slots)
        }


# =============================================================================
# Phase-Weighted Arithmetic (Paradigma 2)
# =============================================================================

class PhaseWeightedALU:
    """
    ALU dove il risultato dipende dalla fase.

    risultato = f(operandi, Φ)

    Lo stesso calcolo produce risultati diversi in momenti diversi,
    ma deterministicamente (se conosci le frequenze).
    """
    def __init__(self, system: TriphaseSystem):
        self.system = system

    def add(self, a: float, b: float, t: float) -> float:
        """Addizione pesata dalla fase AB."""
        phi = self.system.phase_ab(t)
        return a + b * (1.0 + phi)

    def mul(self, a: float, b: float, t: float) -> float:
        """Moltiplicazione modulata dalla fase AO."""
        phi = self.system.phase_ao(t)
        return a * b * (1.0 + 0.5 * math.sin(2 * math.pi * phi))

    def phase_hash(self, x: int, t: float) -> int:
        """Hash che include la fase corrente — non riproducibile senza timing."""
        phi_ab, phi_ao, phi_bo = self.system.phase_vector(t)
        phase_bits = int((phi_ab + 0.5) * 256) & 0xFF
        phase_bits |= (int((phi_ao + 0.5) * 256) & 0xFF) << 8
        phase_bits |= (int((phi_bo + 0.5) * 256) & 0xFF) << 16
        return x ^ phase_bits

    def phase_select(self, values: list, t: float) -> object:
        """Seleziona un valore dalla lista in base alla fase."""
        phi = (self.system.phase_ab(t) + 0.5) % 1.0  # normalize to [0,1)
        index = int(phi * len(values)) % len(values)
        return values[index]


# =============================================================================
# Triphase Virtual Machine
# =============================================================================

class TriphaseVM:
    """
    Macchina virtuale trifasica.

    Combina Paradigma 2 (Phase-Weighted) e 3 (Phase-Encoded State)
    in un modello di esecuzione unificato.
    """
    def __init__(self, system: TriphaseSystem,
                 num_registers: int = 8, slots_per_register: int = 4):
        self.system = system
        self.alu = PhaseWeightedALU(system)
        self.registers = {
            f"r{i}": PhaseRegister(f"r{i}", slots_per_register)
            for i in range(num_registers)
        }
        self.program: list[PhaseInstruction] = []
        self.pc = 0  # program counter
        self.time = 0.0
        self.dt = 1.0 / system.observer.frequency_hz  # observer tick period
        self.log: list[dict] = []

    def load_program(self, instructions: list[PhaseInstruction]):
        self.program = instructions
        self.pc = 0

    def step(self) -> dict:
        """Esegui un tick dell'observer."""
        self.time += self.dt
        phi_vec = self.system.phase_vector(self.time)

        executed = []
        for instr in self.program:
            if instr.can_execute(self.system, self.time):
                result = instr.operation(self, self.time)
                executed.append({
                    "op": instr.op_name,
                    "phase_pair": instr.phase_pair,
                    "result": result
                })

        entry = {
            "tick": self.pc,
            "time": self.time,
            "phases": phi_vec,
            "executed": executed,
            "sync": self.system.is_sync(self.time)
        }
        self.log.append(entry)
        self.pc += 1
        return entry

    def run(self, num_ticks: int) -> list[dict]:
        """Esegui N tick."""
        results = []
        for _ in range(num_ticks):
            results.append(self.step())
        return results

    def read_reg(self, name: str) -> object:
        """Leggi registro alla fase corrente."""
        phi = (self.system.phase_ab(self.time) + 0.5) % 1.0
        return self.registers[name].read(phi)

    def write_reg(self, name: str, value: object):
        """Scrivi registro alla fase corrente."""
        phi = (self.system.phase_ab(self.time) + 0.5) % 1.0
        self.registers[name].write(phi, value)


# =============================================================================
# Preset: M1 Max Approximation
# =============================================================================

def m1_max_system() -> TriphaseSystem:
    """
    Approssimazione dei clock dell'M1 Max.
    Frequenze scalate per simulazione (rapporti preservati).
    """
    return TriphaseSystem(
        alpha=Clock("P-core", 3228e6),    # 3.228 GHz
        beta=Clock("E-core", 2064e6),     # 2.064 GHz
        observer=Clock("Timer", 24e6)     # 24 MHz ARM timer
    )


def simple_system(f_alpha: float = 5.0, f_beta: float = 3.0,
                  f_observer: float = 1.0) -> TriphaseSystem:
    """Sistema semplificato per visualizzazione."""
    return TriphaseSystem(
        alpha=Clock("Alpha", f_alpha),
        beta=Clock("Beta", f_beta),
        observer=Clock("Observer", f_observer)
    )
