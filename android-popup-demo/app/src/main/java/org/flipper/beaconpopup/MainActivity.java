package org.flipper.beaconpopup;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

/**
 * Opt-in BLE demonstration: shows ONE in-app popup only when the user starts
 * scanning and the user's FlipperLab beacon is detected. Does not advertise,
 * pair with, or send data to any other device.
 */
public class MainActivity extends Activity {
    private BluetoothLeScanner scanner;
    private ScanCallback callback;
    private boolean scanning = false;
    private TextView status;
    private Button button;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable scanTimeout = () -> {
        if (scanning) {
            stopScanning();
            status.setText("Nessun FlipperLab rilevato. Controlla che BLE Test sia avviata.");
        }
    };

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(36, 45, 36, 24);

        TextView title = new TextView(this);
        title.setText("FlipperLab - Popup di prova");
        title.setTextSize(22);
        layout.addView(title);

        TextView explanation = new TextView(this);
        explanation.setText("\nSul Flipper apri BLE Test e premi Start.\n" +
                "Premi qui per cercare il beacon: quando viene rilevato, " +
                "apparira' UN popup in questa app.\n");
        explanation.setTextSize(16);
        layout.addView(explanation);

        button = new Button(this);
        button.setText("Cerca FlipperLab");
        button.setOnClickListener(v -> {
            if (scanning) {
                stopScanning();
                status.setText("Ricerca interrotta.");
            } else {
                beginScan();
            }
        });
        layout.addView(button);

        status = new TextView(this);
        status.setText("Pronto. Bluetooth e localizzazione devono essere attivi.");
        status.setTextSize(16);
        layout.addView(status);
        setContentView(layout);
    }

    private boolean requestMissingPermissions() {
        List<String> missing = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 31) {
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED)
                missing.add(Manifest.permission.BLUETOOTH_SCAN);
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED)
                missing.add(Manifest.permission.BLUETOOTH_CONNECT);
        } else if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED)
                missing.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (!missing.isEmpty()) {
            requestPermissions(missing.toArray(new String[0]), 100);
            return true;
        }
        return false;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != 100) return;
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                status.setText("Permesso Bluetooth negato. Consenti l'accesso nelle Impostazioni.");
                return;
            }
        }
        beginScan();
    }

    private void beginScan() {
        if (requestMissingPermissions()) return;

        BluetoothManager manager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager == null ? null : manager.getAdapter();
        if (adapter == null) {
            status.setText("Questo tablet non supporta Bluetooth.");
            return;
        }
        try {
            if (!adapter.isEnabled()) {
                status.setText("Attiva il Bluetooth nelle Impostazioni, poi riprova.");
                return;
            }
            scanner = adapter.getBluetoothLeScanner();
            if (scanner == null) {
                status.setText("Scanner BLE non disponibile.");
                return;
            }

            callback = new ScanCallback() {
                @Override
                public void onScanResult(int callbackType, ScanResult result) {
                    if (!scanning || result == null || result.getScanRecord() == null) return;
                    String name = result.getScanRecord().getDeviceName();
                    if ("FlipperLab".equals(name)) {
                        runOnUiThread(() -> {
                            if (!scanning) return;
                            stopScanning();
                            status.setText("FlipperLab rilevato. Test riuscito.");
                            new AlertDialog.Builder(MainActivity.this)
                                .setTitle("Popup Bluetooth di prova")
                                .setMessage("Rilevato il beacon FlipperLab del tuo Flipper Zero. " +
                                            "Questo popup e' generato localmente dall'app, " +
                                            "non dal sistema di associazione Android.")
                                .setPositiveButton("OK", (dialog, which) -> {})
                                .show();
                        });
                    }
                }

                @Override
                public void onScanFailed(int errorCode) {
                    runOnUiThread(() -> {
                        stopScanning();
                        status.setText("Ricerca BLE non riuscita. Codice: " + errorCode);
                    });
                }
            };
            scanner.startScan(null,
                new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(),
                callback);
            scanning = true;
            button.setText("Interrompi ricerca");
            status.setText("Ricerca BLE in corso (max 20 secondi)...");
            handler.postDelayed(scanTimeout, 20000);
        } catch (SecurityException e) {
            status.setText("Autorizzazioni Bluetooth mancanti.");
        } catch (RuntimeException e) {
            status.setText("Errore scanner: " + e.getClass().getSimpleName());
        }
    }

    private void stopScanning() {
        handler.removeCallbacks(scanTimeout);
        if (!scanning) return;
        scanning = false;
        button.setText("Cerca FlipperLab");
        if (scanner != null && callback != null) {
            try {
                scanner.stopScan(callback);
            } catch (SecurityException ignored) {
            } catch (RuntimeException ignored) {
            }
        }
        callback = null;
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopScanning();
    }

    @Override
    protected void onDestroy() {
        stopScanning();
        super.onDestroy();
    }
}
