# Istruzioni per Deposito UIBM — Brevetto per Invenzione Industriale

## Titolo (in italiano, obbligatorio)
"Metodo e sistema per l'estrazione e l'utilizzo di bit di fase computazionali (phit) dalle relazioni tra domini di clock asincroni nei processori digitali"

## Obbligo linguistico (Guida UIBM, pag. 4-5, 10)

Tutti i documenti devono essere redatti in italiano.
Se depositati in altra lingua: traduzione in italiano entro 2 mesi (termine perentorio, non prorogabile).
Per le domande di invenzione: rivendicazioni tradotte in inglese obbligatorie (oppure pagare EUR 200 per traduzione UIBM).

## Documenti da caricare (tutti PDF separati, firmati digitalmente PAdES)

| # | File | Contenuto | Lingua | Note |
|---|------|-----------|--------|------|
| 1 | `descrizione_FINAL.pdf` | Descrizione completa dell'invenzione | IT | Primario |
| 2 | `rivendicazioni_FINAL.pdf` | Rivendicazioni (10 totali: 3 indipendenti + 7 dipendenti) | IT | Primario |
| 3 | `rivendicazioni_en_FINAL.pdf` | Traduzione inglese delle rivendicazioni | EN | Obbligatorio per brevetti invenzione |
| 4 | `riassunto_FINAL.pdf` | Riassunto (abstract) — foglio separato | IT | Primario |

## Sorgenti e corrispondenza

| PDF | Generato da | Source markdown |
|-----|-------------|---------------|
| descrizione_FINAL.pdf | descrizione_clean.txt | descrizione_it.md |
| rivendicazioni_FINAL.pdf | rivendicazioni_clean.txt | rivendicazioni_it.md |
| rivendicazioni_en_FINAL.pdf | rivendicazioni_en_clean.txt | rivendicazioni.md |
| riassunto_FINAL.pdf | riassunto_clean.txt | riassunto_it.md |

## Accesso
- Portale: https://servizionline.uibm.gov.it
- Autenticazione: SPID o CIE
- Serve: firma digitale CIE (per firmare i PDF, formato PAdES)
- Orario: lunedi-venerdi 8:00-19:00 (esclusi festivi)

## Procedura portale

1. Login SPID/CIE
2. "Nuovo deposito" -> "Brevetto per Invenzione Industriale"
3. Tipo deposito: "Ordinaria" (non rivendichiamo priorita)
4. Compilare dati:
   - Titolo (IT, come sopra)
   - Richiedente: Alessio Cazzaniga, CF, domicilio elettivo (EU/SEE)
   - Inventore: Alessio Cazzaniga, nazionalita italiana
5. Upload 4 PDF firmati digitalmente
6. Classificazione IPC: facoltativa (assegnata dall'EPO nel rapporto di ricerca)
   - Suggerimento se richiesto: G06F 7/58 (Random number generation)
7. Pagamento: pagoPA online contestuale OPPURE F24 entro 1 mese

## Pagamento
- Modalita preferita: pagoPA (online al momento del deposito)
- Alternativa: F24 con elementi identificativi, entro 1 mese
- Importo: **EUR 50** diritti deposito telematico
  - Con 10 rivendicazioni: nessun sovraccosto (soglia = 10)
  - Se si aggiungono rivendicazioni oltre la 10a: EUR 45 ciascuna
- Se F24:
  - Codice ente: UI3
  - Codice tributo: 6936
  - Codice: C300 ("Brevetti e Disegni - deposito, annualita, diritti di opposizione. Altri tributi")
  - Anno di riferimento: 2026

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

## Assistenza tecnica
- Chat: servizionline.uibm.gov.it (lun-ven 9:00-13:00, 14:00-18:00)
- Email: hd1.deposito@mise.gov.it (solo da PEC NON certificata)
- Contact center: contactcenteruibm@mise.gov.it (lun-ven 9:00-17:00)
