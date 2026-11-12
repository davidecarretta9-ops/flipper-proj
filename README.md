# Flipper BLE Test (firmware ufficiale 1.5.1)

App di test per Flipper Zero. Trasmette **un singolo beacon BLE non connettibile** con nome `FlipperLab`, rilevabile con uno scanner come nRF Connect. **Non** esegue BLE Spam, flooding, simulazione di AirPods o popup.

## File
- `ble_spam.c`: sorgente dell'applicazione di test (nome mantenuto per compatibilità con il progetto precedente).
- `application.fam`: configurazione per uFBT.
- `ble_spam_10px.png`: icona.

## Compilare (PC Windows/macOS/Linux)
1. Installa uFBT secondo la guida ufficiale: https://github.com/flipperdevices/flipperzero-ufbt
2. Apri il terminale nella cartella che contiene `application.fam`.
3. Aggiorna l'SDK per il firmware originale 1.5.1: `ufbt update --branch=1.5.1`
4. Esegui `ufbt` per compilare. Il risultato sarà un `.fap` nella cartella `dist/`.

Per installare, trasferisci il file `.fap` in `SD Card/apps/Bluetooth/` tramite qFlipper o l'app mobile.

## Uso
Apri `Apps > Bluetooth > BLE Test`, premi **OK** per iniziare/fermare il beacon. Premi **Back** per uscire. Il beacon può essere identificato cercando il nome `FlipperLab` con un'app scanner BLE.

**Nota:** i sorgenti non sono stati compilati né provati su un Flipper fisico da questa conversazione. Se compare un errore uFBT, condividi l'output della compilazione.
