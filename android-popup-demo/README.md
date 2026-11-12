# Android Popup Demo per Flipper Zero

Questa app Android (6.0 o superiore, con BLE) mostra **un solo popup locale** se rileva il beacon `FlipperLab` del progetto presente nella cartella principale della repository.

Non è BLE Spam: non provoca popup di sistema su altri dispositivi, non impersona accessori Apple o Android, non invia pacchetti agli altri dispositivi.

## Come provarla
1. Scarica l'artefatto `FlipperLab-Android-Popup-Demo` da GitHub Actions > Android Popup Demo e apri lo ZIP.
2. Installa `app-debug.apk` sul tuo tablet Android. Su dispositivi che lo richiedono, consenti l'installazione da questa fonte.
3. Sul Flipper Zero apri `BLE Test` e premi Start. Il beacon si chiama `FlipperLab`.
4. Sul tablet apri `FlipperLab Popup Test`, concedi i permessi Bluetooth / posizione e premi `Cerca FlipperLab`.
5. Se viene rilevato entro 20 secondi, si apre un popup dell'app.

Nota: su Android 6-11 potrebbe essere necessario attivare anche la posizione. Sui tablet più vecchi senza supporto BLE la scansione non funziona. Non è garantito su ogni dispositivo. Il popup compare nell'app, non nel sistema operativo.
