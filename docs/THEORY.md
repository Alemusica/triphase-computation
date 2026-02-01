# Triphase Computation — Theoretical Framework

## Computazione a Fase Estesa (CPE)

### 1. Premessa

Nei computer attuali, l'informazione temporale viene sistematicamente scartata.
Il clock serve solo a dire "ora campiona" — ma *quando esattamente* quel
campionamento avviene rispetto agli altri domini è trattato come rumore
da eliminare, non come segnale.

Eppure quell'informazione **esiste fisicamente**.

### 2. Lo stato convenzionale

In un sistema digitale classico, lo stato al tempo *t* è:

```
S(t) = {s₁, s₂, ..., sₙ}  dove sᵢ ∈ {0, 1}
```

*N* transistor = *N* bit = 2^N stati possibili. L'informazione è puramente spaziale.

### 3. L'estensione: stato + fase

Lo stato completo è:

```
Σ(t) = (S(t), Φ(t))
```

Dove Φ(t) è un vettore di fasi:

```
Φ(t) = (φ₁(t), φ₂(t), ..., φₖ(t))
```

Ogni φᵢ rappresenta la posizione nel ciclo del clock *i*-esimo, normalizzata [0, 1).

### 4. Il battimento come struttura

Con *K* clock a frequenze f₁, f₂, ..., fₖ, il vettore Φ(t) traccia una
traiettoria su un **toro K-dimensionale** (T^K).

I momenti di "sincronia" sono quando:

```
φᵢ(t) ≈ φⱼ(t)  per qualche coppia (i,j)
```

Con clock incommensurabili (rapporti irrazionali), il sistema è
**quasi-periodico** — non torna mai esattamente allo stesso punto,
ma passa arbitrariamente vicino a ogni configurazione (ergodicità).

### 5. L'informazione addizionale

Se il sistema potesse leggere Φ(t), allora due esecuzioni identiche
a livello di *S* potrebbero divergere in base a Φ:

```
f(S, Φ₁) ≠ f(S, Φ₂)
```

L'entropia effettiva del sistema aumenta:

```
H_totale = H(S) + H(Φ|S)
```

Se Φ contribuisce log₂(K) bit distinguibili, gli stati si moltiplicano.

### 6. Architettura triadica

```
Clock A (sorgente)     Clock B (sorgente)
    │                       │
    ▼                       ▼
    ════════════════════════
           BATTIMENTO
         (segnale emergente)
    ════════════════════════
                │
                ▼
           Clock C (osservatore)
                │
                ▼
           CALCOLO
```

- A e B non possono leggere sé stessi
- L'informazione esiste solo nella relazione
- C è l'unico che può "vederla"

### 7. Tre paradigmi computazionali

#### Paradigma 1: Phase-Gated Computation

Il calcolo avviene solo quando Φ è in una certa finestra.

```
if Φ_AB ∈ [θ₁, θ₂]:
    esegui_operazione()
else:
    nop
```

Applicazione: criptografia dove la chiave è il momento giusto.

#### Paradigma 2: Phase-Weighted Arithmetic

Φ diventa un coefficiente nelle operazioni.

```
risultato = a + b × Φ_AB + c × Φ_AC + d × Φ_BC
```

Lo stesso codice produce risultati diversi in momenti diversi,
deterministicamente (se conosci le frequenze).

#### Paradigma 3: Phase-Encoded State

Non memorizzare il valore, memorizza *quando* il valore è valido.

```
x(Φ) = { 42  se Φ ∈ [0.0, 0.3]
       { 17  se Φ ∈ [0.3, 0.7]
       { 99  se Φ ∈ [0.7, 1.0]
```

Un singolo registro contiene molteplici valori multiplexati nel tempo.

### 8. Modello di esecuzione

L'istruzione trifasica primitiva:

```
WHEN Φ_αβ ∈ [window]: WHAT operation → WHERE destination @ phase_tag
```

Tre dimensioni per ogni istruzione:
- Condizione temporale (quando può eseguire)
- Operazione (cosa fa)
- Tag di fase (firma del momento)

### 9. ISA primitiva

| Operazione | Descrizione |
|---|---|
| `PHASE_READ(A,B)` | Leggi Φ_AB nell'istante corrente |
| `PHASE_WAIT(A,B,θ)` | Blocca finché Φ_AB ≈ θ |
| `PHASE_GATE(A,B,θ,op)` | Esegui op solo se Φ_AB ≈ θ |
| `PHASE_MUX(A,B,v₀,v₁)` | Ritorna v₀ se Φ<0.5, v₁ altrimenti |
| `PHASE_HASH(x)` | Hash che include Φ corrente |

### 10. Clock disponibili su Apple M1 Max

| Dominio | Frequenza | Note |
|---|---|---|
| P-cores | 600–3228 MHz | Scala dinamicamente |
| E-cores | 600–2064 MHz | Efficienza |
| GPU | ~1296 MHz | |
| LPDDR5 | 6400 MT/s eff. | ~3200 MHz DDR |
| Timer ARM | 24 MHz | **Fisso, asincrono** |
| RTC | 32.768 kHz | Indipendente, a batteria |
| Audio Clock | 44.1/48 kHz | CoreAudio master |

Il timer ARM a 24 MHz è il candidato principale come "osservatore esterno"
perché è fisso, leggibile in nanosecondi (~41.67 ns), e completamente
asincrono rispetto ai core.

### 11. Densità informativa teorica

Su M1 Max con timer 24 MHz vs CPU 3.228 GHz:

```
Fasi distinguibili ≈ 3228 / 24 = 134.5
Bit extra ≈ log₂(134.5) ≈ 7.1 bit per registro
Con 32 registri: fino a ~227 bit aggiuntivi
```

### 12. Letteratura correlata

- **GALS** (Globally Asynchronous Locally Synchronous): isola i domini, non li sfrutta
- **Race Logic**: usa le "gare" tra segnali come meccanismo computazionale
- **Asynchronous Computing**: circuiti senza clock globale
- **Stochastic Computing**: usa probabilità, non struttura temporale
- **Temporal Coding** (neuroscienze): rate coding + spike timing
- **Spiking Neural Networks**: timing come informazione

### 13. Domanda aperta

> L'informazione ha una dimensione temporale intrinseca che buttiamo via?

Nel modello di Turing, il tempo è sequenza (prima/dopo).
Nel modello trifasico, il tempo è **posizione in uno spazio di fasi** — un indirizzo, non un ordine.

```
La memoria è spaziale.
I calcoli sono temporali.
L'accesso è fase-dipendente.
```
