# Apple BLE Test 2.2 (official Flipper firmware 1.5.1)

This is a **safe replacement, not a binary patch**, for an older `apple_ble_spam.fap` uploaded by the user. The original FAP imports `furi_hal_bt_custom_adv_start/stop/set` symbols; these are not in the supported BLE API for current official firmware. Original classic app source reference: https://github.com/noproto/apple_ble_spam_ofw. This new version is independently implemented with supported `furi_hal_bt_extra_beacon_*` APIs.

## Changes
- Works with **official firmware 1.5.1 API**, without firmware memory scanning, hard-coded function addresses or non-public custom advertising calls.
- Four manually selected modes: AirPods Pro 2 legacy advertisement, AppleTV Nearby Action, iPhone Setup Nearby Action, and non-Apple named BLE signal `FlipperLab`.
- One advertisement type at a time, fixed session MAC, 400ms BLE interval, 30s limit on Apple-shaped tests and unlimited duration only for the neutral `FlipperLab` beacon, modest radio power (~ -3 dBm).
- Error messages displayed on Flipper, Stop / Back controls and automatic cleanup.
- **No** infinite BLE spam, randomized attack rotation, crash packets, or wireless device shutdown attempts.

## Install on Flipper
Use the GitHub Actions artifact `Apple-BLE-Test-Official-1.5.1` and extract `apple_ble_spam.fap`. Place it on microSD under `apps/Bluetooth/`, or install via qFlipper/Flipper Mobile App. This overrides the old app if it had the same ID.

## Usage
1. Open `Apps > Bluetooth > Apple BLE Test` on Flipper.
2. UP/DOWN changes mode when not transmitting. OK starts the advertising. OK stops it early.
3. Keep your iPad awake with Bluetooth enabled. Test only in an isolated area with your own devices.
4. Apple-shaped advertisements stop automatically after 30 seconds. In `FlipperLab` mode, the ordinary BLE beacon continues until you press OK or leave the app.
5. The Apple modes remain bounded; this **does not add repeated pairing prompts or popup flooding**. No guarantee iPadOS displays any popup. Newer versions often ignore such legacy advertisements entirely.

No app needs to be installed on the iPad. **For diagnostic proof of reception**, the `FlipperLab` mode requires an optional BLE scanning app or another BLE scanner; this is not needed for the regular legacy Apple advertisement test.

## Build manually
Run in this directory:

```bash
ufbt update --branch=1.5.1
ufbt
```

The compiled app is in `dist/apple_ble_spam.fap`.

This is a lab demonstration of legacy advertisements, not an actual Apple accessory.
