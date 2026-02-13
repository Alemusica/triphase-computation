# RIVENDICAZIONI / CLAIMS

## RIVENDICAZIONI INDIPENDENTI / INDEPENDENT CLAIMS

**Rivendicazione 1.** Un metodo per l'estrazione di informazione dalla relazione di fase tra domini di clock asincroni in un processore digitale, comprendente:
(a) l'esecuzione di un carico di lavoro computazionale calibrato sul processore, in cui il tempo di esecuzione del carico di lavoro varia con la relazione di fase tra il clock di esecuzione del processore e un clock di un timer di sistema;
(b) la lettura di un valore del timer di sistema dopo il completamento del carico di lavoro;
(c) la combinazione dei bit meno significativi del valore del timer, che sono uniformi in modo dimostrabile a causa della relazione asincrona tra i domini di clock, con una rappresentazione del risultato di esecuzione del carico di lavoro per produrre un valore dipendente dalla fase;
(d) l'applicazione di una funzione hash al valore dipendente dalla fase per produrre un campione phit (phase-bit);
(e) opzionalmente, la ripetizione dei passi da (a) a (d) con seed del carico di lavoro variati e l'accumulo dei risultati mediante operazioni di rotazione e or-esclusivo per produrre un campione phit composto con contenuto informativo superiore a quello di un singolo campione.

**Rivendicazione 2.** Un metodo per il routing lock-free di task in un sistema multiprocessore, comprendente:
(a) l'estrazione di un campione phit secondo il metodo della Rivendicazione 1;
(b) il calcolo di un identificatore di worker come il campione phit modulo il numero di worker disponibili;
(c) l'instradamento del task al worker identificato;
in cui nessun mutex, contatore atomico o memoria condivisa viene acceduto durante l'operazione di routing, e la distribuzione uniforme dei campioni di fase garantisce una distribuzione statisticamente uniforme dei task tra i worker.

**Rivendicazione 3.** Un'istruzione per processore (RDPHI) per la lettura della relazione di fase istantanea tra due domini di clock asincroni specificati, comprendente:
(a) un circuito comparatore di fase implementato come un flip-flop che campiona il contatore di un dominio di clock sul fronte di salita di un altro dominio di clock;
(b) la memorizzazione del valore di fase campionato in un registro general-purpose;
(c) in cui il valore di fase è disponibile per la computazione general-purpose, inclusa la ponderazione aritmetica, il gating condizionale e la codifica di stato, e non è limitato all'estrazione di entropia o alla generazione di numeri casuali.

## RIVENDICAZIONI DIPENDENTI / DEPENDENT CLAIMS

**Rivendicazione 4.** Il metodo della Rivendicazione 1, in cui il campionamento composto utilizza N=2 letture sequenziali come compromesso ottimale tra resa in phit e throughput, producendo circa 4,06 phit per campione composto rispetto a 1,96 phit per singolo campione.

**Rivendicazione 5.** Il metodo della Rivendicazione 1, in cui l'uniformità dei bit meno significativi del timer è validata da un test Chi-quadro che produce un valore inferiore alla soglia critica per il numero di bin, confermando la relazione asincrona tra i domini di clock.

**Rivendicazione 6.** Il metodo della Rivendicazione 1, comprendente inoltre l'accumulo di campioni phit in un pool di entropia di N corsie da M bit ciascuna, utilizzando hashing moltiplicativo con costanti del rapporto aureo, mixing SplitMix64 e rotazione tra corsie, e l'estrazione di valori casuali mediante la combinazione di tutte le corsie con or-esclusivo ruotato e la mutazione dello stato del pool dopo l'estrazione per la forward security.

**Rivendicazione 7.** Il metodo della Rivendicazione 1, comprendente inoltre la derivazione di un keystream crittografico dal vettore di fase Phi(t) = (f_1*t mod 1, f_2*t mod 1, ..., f_K*t mod 1) a uno specifico tempo t, dove f_1, f_2, ..., f_K sono le frequenze di K domini di clock asincroni, e la cifratura di un messaggio in chiaro mediante or-esclusivo con il keystream, in cui la decifratura richiede la conoscenza sia delle frequenze di clock esatte sia del momento esatto della cifratura.

**Rivendicazione 8.** Il metodo della Rivendicazione 7, comprendente inoltre finestre di accesso phase-locked in cui la decifratura è possibile solo quando la fase relativa tra una coppia specificata di domini di clock rientra in un intervallo predeterminato.

**Rivendicazione 9.** L'istruzione della Rivendicazione 3, comprendente inoltre istruzioni complementari PHIGATE e PHIWEIGHT che rispettivamente condizionano l'esecuzione su una condizione di fase e restituiscono la fase come peso aritmetico a virgola fissa, abilitando tre paradigmi computazionali: computazione phase-gated, computazione phase-weighted e computazione phase-encoded.

**Rivendicazione 10.** Una libreria software portabile che implementa i metodi delle Rivendicazioni 1, 2, 6 e 7 come libreria C header-only con backend timer specifici per piattaforma per architetture ARM, x86 e RISC-V, fornendo estrazione di phit, campionamento composto, gestione del pool di entropia, PRNG, routing lock-free e funzionalità di auto-test senza richiedere modifiche hardware.
