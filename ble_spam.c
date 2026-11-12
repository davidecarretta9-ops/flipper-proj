/**
 * Flipper Zero BLE test beacon - official firmware 1.5.1.
 * Single non-connectable BLE beacon named FlipperLab; no spam or spoofing.
 * The entry point name "ble_spam" is retained for compatibility only.
 */

#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_random.h>
#include <extra_beacon.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TAG "BleLab"

static const uint8_t test_advertisement[] = {
    0x02, 0x01, 0x06,
    0x0B, 0x09, 'F', 'l', 'i', 'p', 'p', 'e', 'r', 'L', 'a', 'b',
};

typedef enum {
    BeaconStatusReady,
    BeaconStatusBroadcasting,
    BeaconStatusBusy,
    BeaconStatusConfigError,
    BeaconStatusDataError,
    BeaconStatusStartError,
    BeaconStatusStopError,
} BeaconStatus;

typedef struct {
    bool advertising;
    bool has_previous_config;
    GapExtraBeaconConfig previous_config;
    uint8_t previous_data[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t previous_len;
    BeaconStatus status;
} BeaconState;

static const char* beacon_status_string(BeaconStatus status) {
    switch(status) {
    case BeaconStatusReady:
        return "Ready";
    case BeaconStatusBroadcasting:
        return "Broadcasting";
    case BeaconStatusBusy:
        return "Beacon radio busy";
    case BeaconStatusConfigError:
        return "BLE config failed";
    case BeaconStatusDataError:
        return "BLE data failed";
    case BeaconStatusStartError:
        return "BLE start failed";
    case BeaconStatusStopError:
        return "BLE stop failed";
    default:
        return "Unknown state";
    }
}

static void beacon_restore(BeaconState* state) {
    if(!state || !state->has_previous_config) return;
    if(!furi_hal_bt_extra_beacon_set_config(&state->previous_config)) {
        FURI_LOG_W(TAG, "Could not restore previous config");
        return;
    }
    if(!furi_hal_bt_extra_beacon_set_data(state->previous_data, state->previous_len)) {
        FURI_LOG_W(TAG, "Could not restore previous data");
    }
}

static bool beacon_stop(BeaconState* state) {
    if(!state) return false;
    if(!state->advertising) return true;
    if(!furi_hal_bt_extra_beacon_stop()) {
        state->status = BeaconStatusStopError;
        FURI_LOG_E(TAG, "Failed to stop BLE beacon");
        return false;
    }
    state->advertising = false;
    beacon_restore(state);
    state->status = BeaconStatusReady;
    return true;
}

static bool beacon_start(BeaconState* state) {
    if(!state) return false;
    if(state->advertising) return true;

    // Do not interfere with another app's existing extra beacon.
    if(furi_hal_bt_extra_beacon_is_active()) {
        state->status = BeaconStatusBusy;
        return false;
    }

    const GapExtraBeaconConfig* old_config = furi_hal_bt_extra_beacon_get_config();
    state->has_previous_config = (old_config != NULL);
    if(old_config) {
        memcpy(&state->previous_config, old_config, sizeof(state->previous_config));
    }
    state->previous_len = furi_hal_bt_extra_beacon_get_data(state->previous_data);

    GapExtraBeaconConfig config = {
        .min_adv_interval_ms = 1000,
        .max_adv_interval_ms = 1200,
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = GapAdvPowerLevel_Neg10_9dBm,
        .address_type = GapAddressTypeRandom,
        .address = {0},
    };

    furi_hal_random_fill_buf(config.address, sizeof(config.address));
    config.address[5] = (config.address[5] & 0x3F) | 0xC0;

    if(!furi_hal_bt_extra_beacon_set_config(&config)) {
        state->status = BeaconStatusConfigError;
        return false;
    }

    if(!furi_hal_bt_extra_beacon_set_data(test_advertisement, sizeof(test_advertisement))) {
        state->status = BeaconStatusDataError;
        beacon_restore(state);
        return false;
    }

    if(!furi_hal_bt_extra_beacon_start()) {
        state->status = BeaconStatusStartError;
        beacon_restore(state);
        return false;
    }

    state->advertising = true;
    state->status = BeaconStatusBroadcasting;
    return true;
}

static void draw_callback(Canvas* canvas, void* context) {
    const BeaconState* state = context;
    if(!state) return;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 12, "BLE Test");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 26, "Name: FlipperLab");
    canvas_draw_str(canvas, 4, 38, beacon_status_string(state->status));
    canvas_draw_str(canvas, 4, 49, "Scanner: nRF Connect");

    elements_button_center(canvas, state->advertising ? "Stop" : "Start");
    elements_button_left(canvas, "Exit");
}

static void input_callback(InputEvent* event, void* context) {
    FuriMessageQueue* queue = context;
    if(event && queue && event->type == InputTypeShort) {
        furi_message_queue_put(queue, event, 0);
    }
}

int32_t ble_spam(void* args) {
    UNUSED(args);

    BeaconState* state = calloc(1, sizeof(BeaconState));
    if(!state) return -1;
    state->status = BeaconStatusReady;

    FuriMessageQueue* input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    if(!input_queue) {
        free(state);
        return -1;
    }

    ViewPort* view_port = view_port_alloc();
    if(!view_port) {
        furi_message_queue_free(input_queue);
        free(state);
        return -1;
    }

    Gui* gui = furi_record_open(RECORD_GUI);
    view_port_input_callback_set(view_port, input_callback, input_queue);
    view_port_draw_callback_set(view_port, draw_callback, state);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    bool running = true;
    while(running) {
        InputEvent event;
        if(furi_message_queue_get(input_queue, &event, FuriWaitForever) != FuriStatusOk) {
            continue;
        }

        if(event.key == InputKeyOk) {
            if(state->advertising) {
                beacon_stop(state);
            } else {
                beacon_start(state);
            }
        } else if(event.key == InputKeyBack || event.key == InputKeyLeft) {
            // If stopping fails, keep UI open to show the error.
            if(beacon_stop(state)) running = false;
        }
        view_port_update(view_port);
    }

    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(input_queue);
    free(state);
    return 0;
}
