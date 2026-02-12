# Istruzioni per Deposito UIBM — Brevetto per Invenzione Industriale

## Titolo (in italiano, obbligatorio)
"Metodo e sistema per l'estrazione e l'utilizzo di bit di fase computazionali (phit) dalle relazioni tra domini di clock asincroni nei processori digitali"

## Documenti da caricare (tutti PDF separati, firmati digitalmente)

| # | File | Contenuto | Lingua |
|---|------|-----------|--------|
| 1 | `descrizione.pdf` | Descrizione completa dell'invenzione | EN |
| 2 | `rivendicazioni.pdf` | Rivendicazioni (10 totali: 3 indipendenti + 7 dipendenti) | EN |
| 3 | `rivendicazioni_en.pdf` | Traduzione inglese delle rivendicazioni (= identico al file 2, gia in inglese) | EN |
| 4 | `riassunto.pdf` | Riassunto (abstract) — foglio separato | EN |
| 5 | `disegni.pdf` | Tavole dei disegni (7 figure) — facoltativo ma consigliato | - |

## Accesso
- Portale: https://servizionline.uibm.gov.it
- Autenticazione: SPID o CIE
- Serve: firma digitale CIE (per firmare i PDF, formato PAdES)

## Pagamento
- Modalita: F24 con elementi identificativi
- Importo: **EUR 50** diritti deposito telematico
  - Con 10 rivendicazioni: nessun sovraccosto (soglia = 10)
  - Se si aggiungono rivendicazioni oltre la 10a: EUR 45 ciascuna
- Codice ente: UI3
- Codice tributo per diritti di deposito brevetto: 6936
- Termine pagamento: entro 1 mese dalla presentazione

## Nota costi — perche 10 rivendicazioni

UIBM applica EUR 45 per ogni rivendicazione oltre la 10a.
Con 10 rivendicazioni: EUR 0 aggiuntivi.
Le rivendicazioni sono state strutturate come 3 indipendenti + 7 dipendenti
per massimizzare la copertura entro il limite di 10.

Rivendicazioni indipendenti:
1. Metodo di estrazione phit (core invention)
2. Routing lock-free via fase (strongest novelty — no prior art found)
3. Istruzione RDPHI per lettura fase hardware (ISA extension)

## Classificazione IPC suggerita

- G06F 1/04 (Generating or distributing clock signals)
- G06F 7/58 (Random or pseudo-random number generation)
- H04L 9/08 (Key distribution, using clock signals)
- G06F 9/46 (Arrangements for multiprocessing — scheduling)

## Dopo il deposito
- Ricevi ricevuta via email con numero di domanda
- La domanda resta segreta per 18 mesi
- Entro 9 mesi: rapporto di ricerca EPO (gratuito)
- Data di priorita: fissata al momento del deposito
- Da quel momento puoi pubblicare (arXiv, Reddit, repo pubblico)
- Hai 12 mesi dalla data di deposito per estendere all'estero (USPTO, EPO, PCT)

## Prior art da citare (aggiunta rispetto al provisional US)
- US10498528B2 (Clock Machines, Kwok, 2019) — differenziato per multi-clock async vs single-clock discrete
- RISC-V entropy source interface (Neumann et al., 2022) — differenziato per computation vs entropy-only
- Jitterentropy (Muller) — differenziato per structured phase vs random noise, 4x yield

## Note
- Le rivendicazioni sono gia in inglese -> non serve pagare EUR 200 per traduzione
- Descrizione e rivendicazioni possono essere in inglese
- Solo il titolo DEVE essere in italiano
- Ogni pagina deve essere numerata e firmata digitalmente
