/**
 * Apple proximity pairing demo for Flipper Zero official firmware 1.5.1.
 *
 * TEST ONLY: emits one type of legacy BLE accessory advertisement for up to
 * 10 seconds, with low RF power and 500ms advertising interval.
 *
 * No cycling/randomizing fake models, repeated bursts, crash payloads,
 * flooding, or pairing/connection attempts. There is no guarantee recent
 * iPadOS will display a system pairing popup.
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

#define TAG "ApplePopupLab"
#define TEST_DURATION_MS (10000U)

typedef enum {
    StatusReady,
    StatusAdvertising,
    StatusBusy,
    StatusConfigFail,
    StatusDataFail,
    StatusStartFail,
    StatusStopFail,
} Status;

typedef struct {
    bool advertising;
    bool previous_saved;
    GapExtraBeaconConfig previous_config;
    uint8_t previous_data[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t previous_length;
    uint32_t started_at;
    Status status;
} LabState;

/*
 * Legacy Apple Continuity ProximityPair advertisement, AirPods Pro 2 model.
 * 31-byte single AD structure, manufacturer-specific type 0xFF, Apple 0x004c,
 * continuity subtype 0x07 length 0x19.
 *
 * This does not create a genuine AirPods device or permit pairing.
 * Newer iPadOS may ignore it entirely.
 */
static void make_demo_packet(uint8_t packet[31]) {
    static const uint8_t header[] = {
        0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19,
        0x01, 0x14, 0x20, 0x55,
        0x55, 0x55, 0x01, 0x00, 0x00,
    };
    memcpy(packet, header, sizeof(header));
    furi_hal_random_fill_buf(packet + sizeof(header), 31 - sizeof(header));
}

static const char* status_text(Status status) {
    switch(status) {
    case StatusReady: return "Ready - press OK";
    case StatusAdvertising: return "Test transmitting...";
    case StatusBusy: return "Other BLE beacon active";
    case StatusConfigFail: return "Configuration failed";
    case StatusDataFail: return "Payload not accepted";
    case StatusStartFail: return "Bluetooth start failed";
    case StatusStopFail: return "Bluetooth stop failed";
    default: return "Unknown";
    }
}

static void restore_previous(LabState* state) {
    if(!state->previous_saved) return;
    if(furi_hal_bt_extra_beacon_set_config(&state->previous_config)) {
        (void)furi_hal_bt_extra_beacon_set_data(
            state->previous_data, state->previous_length);
    }
    state->previous_saved = false;
}

static bool stop_test(LabState* state) {
    if(!state->advertising) return true;
    if(!furi_hal_bt_extra_beacon_stop()) {
        state->status = StatusStopFail;
        FURI_LOG_E(TAG, "Could not stop beacon");
        return false;
    }
    state->advertising = false;
    restore_previous(state);
    state->status = StatusReady;
    return true;
}

static bool start_test(LabState* state) {
    if(state->advertising) return true;
    if(furi_hal_bt_extra_beacon_is_active()) {
        state->status = StatusBusy;
        return false;
    }

    const GapExtraBeaconConfig* old = furi_hal_bt_extra_beacon_get_config();
    state->previous_saved = old != NULL;
    if(old) memcpy(&state->previous_config, old, sizeof(state->previous_config));
    state->previous_length = furi_hal_bt_extra_beacon_get_data(state->previous_data);

    GapExtraBeaconConfig config = {
        .min_adv_interval_ms = 500,
        .max_adv_interval_ms = 500,
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = GapAdvPowerLevel_Neg20_85dBm,
        .address_type = GapAddressTypeRandom,
        .address = {0},
    };
    furi_hal_random_fill_buf(config.address, sizeof(config.address));
    // Static random address: top two bits must be 1.
    config.address[5] = (config.address[5] & 0x3F) | 0xC0;

    if(!furi_hal_bt_extra_beacon_set_config(&config)) {
        state->status = StatusConfigFail;
        state->previous_saved = false;
        return false;
    }

    uint8_t data[31];
    make_demo_packet(data);
    if(!furi_hal_bt_extra_beacon_set_data(data, sizeof(data))) {
        state->status = StatusDataFail;
        restore_previous(state);
        return false;
    }
    if(!furi_hal_bt_extra_beacon_start()) {
        state->status = StatusStartFail;
        restore_previous(state);
        return false;
    }
    state->advertising = true;
    state->started_at = furi_get_tick();
    state->status = StatusAdvertising;
    return true;
}

static void draw_callback(Canvas* canvas, void* context) {
    LabState* state = context;
    if(!state) return;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 3, 12, "Apple Popup Lab");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 3, 25, "AirPods demo / 10 sec");
    canvas_draw_str(canvas, 3, 38, status_text(state->status));
    if(state->advertising) {
        uint32_t elapsed = furi_get_tick() - state->started_at;
        uint32_t remaining = elapsed < TEST_DURATION_MS ? (TEST_DURATION_MS - elapsed + 999) / 1000 : 0;
        char buffer[24];
        snprintf(buffer, sizeof(buffer), "Auto stop in: %lus", (unsigned long)remaining);
        canvas_draw_str(canvas, 3, 49, buffer);
    } else {
        canvas_draw_str(canvas, 3, 49, "Low power / one payload");
    }
    elements_button_center(canvas, state->advertising ? "Stop" : "Start");
    elements_button_left(canvas, "Exit");
}

static void input_callback(InputEvent* input, void* context) {
    FuriMessageQueue* queue = context;
    if(input && queue && input->type == InputTypeShort) {
        (void)furi_message_queue_put(queue, input, 0);
    }
}

int32_t apple_popup_lab(void* args) {
    UNUSED(args);
    LabState* state = calloc(1, sizeof(*state));
    if(!state) return -1;
    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    if(!queue) {
        free(state);
        return -1;
    }
    ViewPort* view = view_port_alloc();
    if(!view) {
        furi_message_queue_free(queue);
        free(state);
        return -1;
    }

    Gui* gui = furi_record_open(RECORD_GUI);
    view_port_input_callback_set(view, input_callback, queue);
    view_port_draw_callback_set(view, draw_callback, state);
    gui_add_view_port(gui, view, GuiLayerFullscreen);

    bool running = true;
    while(running) {
        InputEvent event;
        FuriStatus result = furi_message_queue_get(queue, &event, 100);
        if(state->advertising && (uint32_t)(furi_get_tick() - state->started_at) >= TEST_DURATION_MS) {
            stop_test(state);
        }
        if(result == FuriStatusOk) {
            if(event.key == InputKeyOk) {
                if(state->advertising) stop_test(state);
                else start_test(state);
            } else if(event.key == InputKeyBack || event.key == InputKeyLeft) {
                if(stop_test(state)) running = false;
            }
        }
        view_port_update(view);
    }

    gui_remove_view_port(gui, view);
    furi_record_close(RECORD_GUI);
    view_port_free(view);
    furi_message_queue_free(queue);
    free(state);
    return 0;
}
