# Apple Popup Lab — Flipper Zero, firmware ufficiale 1.5.1

Esperimento di prossimità BLE per **il tuo iPad/iPhone**, senza installare software sul dispositivo ricevente.

Trasmette **un solo tipo** di annuncio BLE con struttura storicamente usata da accessori Apple (AirPods Pro 2). Non crea un accessorio reale, non permette la connessione o l'abbinamento, non accede a dati, non invia messaggi e non contiene sequenze crash. Durata massima 10 secondi, intervallo 500 ms, potenza radio bassa, stop manuale o automatico.

**Non è BLE Spam**. Può essere ignorato dai dispositivi Apple aggiornati. Non aspettarti un popup su tutte le versioni di iPadOS.

## Test

1. Sul Flipper (firmware ufficiale 1.5.1) apri `Apps > Bluetooth > Apple Popup Lab`.
2. Sul tuo iPad attiva Bluetooth e mostra la schermata Home (sblocca il display).
3. Avvicina fisicamente i due dispositivi; assicurati che non ci siano dispositivi non consenzienti nelle immediate vicinanze.
4. Premi OK sul Flipper. L'annuncio si interrompe automaticamente dopo 10 secondi. OK termina prima.
5. Non comparirà nulla sui firmware che ignorano tali annunci.

## Compilazione

```bash
ufbt update --branch=1.5.1
ufbt
```

Il file `.fap` viene prodotto nella directory `dist`. Il workflow GitHub Actions `Apple Popup Lab` allega l'artefatto compilato dopo una build riuscita.

La compilazione non garantisce che il popup compaia. Se il Flipper mostra un errore, interrompi il test e allega il registro del crash.
