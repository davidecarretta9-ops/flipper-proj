/*
 * Apple BLE Test (safe update for official Flipper Zero firmware 1.5.1)
 *
 * Replacement for older Apple BLE Spam FAPs using obsolete custom_adv APIs.
 * Uses only supported furi_hal_bt_extra_beacon APIs.
 *
 * Manual test only, one advertisement format at a time, 12-second automatic
 * stop, 400ms interval, modest TX power. No unlimited flood / crash packets.
 * iPadOS may ignore legacy Continuity advertisements.
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
#include <stdio.h>

#define TAG "AppleBleTest"
#define MAX_DURATION_MS 12000U
#define MODE_COUNT 4U

typedef enum {
    ModeAirPods = 0,
    ModeAppleTv = 1,
    ModeIphoneSetup = 2,
    ModeBeacon = 3,
} TestMode;

typedef enum {
    StatusReady,
    StatusOn,
    StatusBusy,
    StatusConfigFailed,
    StatusDataFailed,
    StatusStartFailed,
    StatusStopFailed,
} TestStatus;

typedef struct {
    TestMode mode;
    TestStatus status;
    bool on;
    bool restore_valid;
    GapExtraBeaconConfig previous_config;
    uint8_t previous_data[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t previous_length;
    uint32_t started_at;
} AppState;

static const char* const mode_labels[MODE_COUNT] = {
    "AirPods Pro 2",
    "AppleTV pairing",
    "iPhone setup",
    "Signal: FlipperLab",
};

/*
 * The advertising data is a BLE AD structure, not a real AirPods/AppleTV
 * peripheral. The Apple Continuity values come from publicly documented
 * legacy advertisements. This experiment does not attempt pairing.
 */
static void make_data(TestMode mode, uint8_t* out, uint8_t* out_size) {
    if(mode == ModeAirPods) {
        const uint8_t header[] = {
            0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19,
            0x01, 0x14, 0x20, 0x55, 0x66, 0x55,
            0x01, 0x00, 0x00,
        };
        memcpy(out, header, sizeof(header));
        furi_hal_random_fill_buf(out + sizeof(header), 31U - sizeof(header));
        *out_size = 31;
    } else if(mode == ModeAppleTv || mode == ModeIphoneSetup) {
        out[0] = 0x0A; // Length = remaining 10 bytes.
        out[1] = 0xFF; // Manufacturer specific
        out[2] = 0x4C; // Apple Company ID 0x004C (little endian)
        out[3] = 0x00;
        out[4] = 0x0F; // Continuity: Nearby Action
        out[5] = 0x05; // Subtype payload length
        out[6] = 0xC0;
        out[7] = (mode == ModeAppleTv) ? 0x20 : 0x09;
        furi_hal_random_fill_buf(out + 8, 3);
        *out_size = 11;
    } else {
        const uint8_t beacon[] = {
            0x02, 0x01, 0x06,
            0x0B, 0x09, 'F', 'l', 'i', 'p', 'p', 'e', 'r', 'L', 'a', 'b'
        };
        memcpy(out, beacon, sizeof(beacon));
        *out_size = sizeof(beacon);
    }
}

static const char* describe_status(TestStatus s) {
    switch(s) {
    case StatusReady: return "Ready / UP DOWN select";
    case StatusOn: return "Advertising (auto-stop)";
    case StatusBusy: return "Beacon already in use";
    case StatusConfigFailed: return "BLE config error";
    case StatusDataFailed: return "BLE payload error";
    case StatusStartFailed: return "BLE start error";
    case StatusStopFailed: return "BLE stop error";
    default: return "Unknown error";
    }
}

static void restore_config(AppState* st) {
    if(!st->restore_valid) return;
    if(furi_hal_bt_extra_beacon_set_config(&st->previous_config)) {
        furi_hal_bt_extra_beacon_set_data(st->previous_data, st->previous_length);
    }
    st->restore_valid = false;
}

static bool stop_advertising(AppState* st) {
    if(!st->on) return true;
    if(!furi_hal_bt_extra_beacon_stop()) {
        st->status = StatusStopFailed;
        return false;
    }
    st->on = false;
    restore_config(st);
    st->status = StatusReady;
    return true;
}

static bool start_advertising(AppState* st) {
    if(st->on) return true;
    if(furi_hal_bt_extra_beacon_is_active()) {
        st->status = StatusBusy;
        return false;
    }

    const GapExtraBeaconConfig* prev = furi_hal_bt_extra_beacon_get_config();
    st->restore_valid = (prev != NULL);
    if(prev) memcpy(&st->previous_config, prev, sizeof(st->previous_config));
    st->previous_length = furi_hal_bt_extra_beacon_get_data(st->previous_data);

    GapExtraBeaconConfig config = {
        .min_adv_interval_ms = 400,
        .max_adv_interval_ms = 400,
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = GapAdvPowerLevel_Neg3_15dBm,
        .address_type = GapAddressTypeRandom,
        .address = {0},
    };
    furi_hal_random_fill_buf(config.address, sizeof(config.address));
    config.address[5] = (config.address[5] & 0x3FU) | 0xC0U;

    if(!furi_hal_bt_extra_beacon_set_config(&config)) {
        st->restore_valid = false;
        st->status = StatusConfigFailed;
        return false;
    }

    uint8_t ad[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t ad_size = 0;
    make_data(st->mode, ad, &ad_size);

    if(!furi_hal_bt_extra_beacon_set_data(ad, ad_size)) {
        st->status = StatusDataFailed;
        restore_config(st);
        return false;
    }

    if(!furi_hal_bt_extra_beacon_start()) {
        st->status = StatusStartFailed;
        restore_config(st);
        return false;
    }
    st->started_at = furi_get_tick();
    st->on = true;
    st->status = StatusOn;
    return true;
}

static void draw_cb(Canvas* c, void* ctx) {
    AppState* st = ctx;
    canvas_clear(c);
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 3, 12, "Apple BLE Test 1.5.1");
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 3, 24, mode_labels[st->mode]);
    canvas_draw_str(c, 3, 36, describe_status(st->status));

    char info[40];
    if(st->on) {
        uint32_t elapsed = furi_get_tick() - st->started_at;
        uint32_t left = elapsed < MAX_DURATION_MS ? (MAX_DURATION_MS - elapsed + 999U) / 1000U : 0;
        snprintf(info, sizeof(info), "Stop in %lus | 400ms",
                 (unsigned long)left);
    } else {
        snprintf(info, sizeof(info), "LOW RATE / 12s MAX");
    }
    canvas_draw_str(c, 3, 48, info);

    elements_button_center(c, st->on ? "Stop" : "Start");
    elements_button_left(c, "Exit");
}

static void input_cb(InputEvent* event, void* ctx) {
    if(event && event->type == InputTypeShort) {
        furi_message_queue_put((FuriMessageQueue*)ctx, event, 0);
    }
}

int32_t apple_ble_spam(void* args) {
    UNUSED(args);
    AppState* st = calloc(1, sizeof(AppState));
    if(!st) return -1;

    FuriMessageQueue* events = furi_message_queue_alloc(8, sizeof(InputEvent));
    if(!events) {
        free(st);
        return -1;
    }

    ViewPort* vp = view_port_alloc();
    if(!vp) {
        furi_message_queue_free(events);
        free(st);
        return -1;
    }

    Gui* gui = furi_record_open(RECORD_GUI);
    view_port_draw_callback_set(vp, draw_cb, st);
    view_port_input_callback_set(vp, input_cb, events);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    bool running = true;
    while(running) {
        InputEvent event;
        FuriStatus got = furi_message_queue_get(events, &event, 100);
        if(st->on && (uint32_t)(furi_get_tick() - st->started_at) >= MAX_DURATION_MS) {
            stop_advertising(st);
        }

        if(got == FuriStatusOk) {
            if(event.key == InputKeyOk) {
                if(st->on) stop_advertising(st);
                else start_advertising(st);
            } else if(!st->on && event.key == InputKeyUp) {
                st->mode = (st->mode + MODE_COUNT - 1) % MODE_COUNT;
                st->status = StatusReady;
            } else if(!st->on && event.key == InputKeyDown) {
                st->mode = (st->mode + 1) % MODE_COUNT;
                st->status = StatusReady;
            } else if(event.key == InputKeyBack || event.key == InputKeyLeft) {
                if(stop_advertising(st)) running = false;
            }
        }

        view_port_update(vp);
    }

    gui_remove_view_port(gui, vp);
    furi_record_close(RECORD_GUI);
    view_port_free(vp);
    furi_message_queue_free(events);
    free(st);
    return 0;
}
