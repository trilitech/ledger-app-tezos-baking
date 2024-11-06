/* Tezos Ledger application - Common BAGL UI functions

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2019 Obsidian Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#ifdef HAVE_BAGL
#include "bolos_target.h"

#include "ui.h"

#include "baking_auth.h"
#include "exception.h"
#include "globals.h"
#include "glyphs.h"  // ui-menu
#include "keys.h"
#include "memory.h"
#include "os_cx.h"  // ui-menu
#include "to_string.h"
#include "ui_screensaver.h"

#include <stdbool.h>
#include <string.h>

#define G_display global.dynamic_display

#ifdef TARGET_NANOS
#include "io.h"

void ux_set_low_cost_display_mode(bool enable) {
    if (G_display.low_cost_display_mode != enable) {
        G_display.low_cost_display_mode = enable;
        if (G_display.low_cost_display_mode) {
            ux_screensaver_start_clock();
        } else {
            ux_screensaver_stop();
        }
    }
}

uint8_t io_event(uint8_t channel);

/**
 * Function similar to the one in `lib_standard_app/` except that the
 * `TICKER_EVENT` handling is not enabled on low-cost display mode.
 *
 * Low-cost display mode is deactivated when the button is pressed and
 * activated during signing because `TICKER_EVENT` handling slows down
 * the application.
 *
 */
uint8_t io_event(uint8_t channel) {
    (void) channel;

    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            // Pressing any button will stop the low-cost display mode.
            ux_set_low_cost_display_mode(false);
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;
        case SEPROXYHAL_TAG_STATUS_EVENT:
            if ((G_io_apdu_media == IO_APDU_MEDIA_USB_HID) &&
                !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
                  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            __attribute__((fallthrough));
        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            // As soon as something is newly displayed, the low-cost display mode stops.
            ux_set_low_cost_display_mode(false);
#ifdef HAVE_BAGL
            UX_DISPLAYED_EVENT({});
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
            UX_DEFAULT_EVENT();
#endif  // HAVE_NBGL
            break;
#ifdef HAVE_NBGL
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#endif  // HAVE_NBGL
        case SEPROXYHAL_TAG_TICKER_EVENT:
            if (!G_display.low_cost_display_mode) {
                app_ticker_event_callback();
                UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
            } else {
                ux_screensaver_apply_tick();
            }
            break;
        default:
            UX_DEFAULT_EVENT();
            break;
    }

    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    return 1;
}

#endif  // TARGET_NANOS

static void ui_refresh_idle_hwm_screen(void);

/**
 * @brief This structure represents a context needed for home screens navigation
 *
 */
typedef struct {
    char chain_id[CHAIN_ID_BASE58_STRING_SIZE];
    char authorized_key[PKH_STRING_SIZE];
    char hwm[MAX_INT_DIGITS + 1u + MAX_INT_DIGITS + 1u];
    char hwm_status[HWM_STATUS_SIZE];
} HomeContext_t;

/// Current home context
static HomeContext_t home_context;

void ui_settings(void);    ///> Initialize settings page
void ui_toggle_hwm(void);  ///> Toggle HWM settings
void ui_menu_init(void);   ///> Load main menu page
/**
 * @brief Idle flow
 *
 *        - Home screen
 *        - Version screen
 *        - Chain-id screen
 *        - Public key hash screen
 *        - High Watermark screen
 *        - Exit screen
 *
 */
#ifdef TARGET_NANOS
UX_STEP_CB(ux_app_is_ready_step, nn, ui_start_screensaver(), {"Application", "is ready"});
#else   // TARGET_NANOS
UX_STEP_NOCB(ux_app_is_ready_step, nn, {"Application", "is ready"});
#endif  // TARGET_NANOS
UX_STEP_NOCB(ux_version_step, bnnn_paging, {"Tezos Baking", APPVERSION});
UX_STEP_NOCB(ux_chain_id_step, bnnn_paging, {"Chain", home_context.chain_id});
UX_STEP_NOCB(ux_authorized_key_step, bnnn_paging, {"Public Key Hash", home_context.authorized_key});
UX_STEP_CB(ux_settings_step, pb, ui_settings(), {&C_icon_coggle, "Settings"});
UX_STEP_CB(ux_hwm_step,
           bnnn_paging,
           ui_refresh_idle_hwm_screen(),
           {"High Watermark", home_context.hwm});
UX_STEP_CB(ux_idle_quit_step, pb, app_exit(), {&C_icon_dashboard_x, "Quit"});

UX_FLOW(ux_idle_flow,
        &ux_app_is_ready_step,
        &ux_version_step,
        &ux_chain_id_step,
        &ux_authorized_key_step,
        &ux_hwm_step,
        &ux_settings_step,
        &ux_idle_quit_step,
        FLOW_LOOP);

void ui_menu_init(void) {
    ux_flow_init(0, ux_idle_flow, NULL);
}

UX_STEP_CB(ux_hwm_info, bn, ui_toggle_hwm(), {"High Watermark", home_context.hwm_status});
UX_STEP_CB(ux_menu_back_step, pb, ui_menu_init(), {&C_icon_back, "Back"});

// FLOW for the about submenu:
// #1 screen: app info
// #2 screen: back button to main menu
UX_FLOW(ux_settings_flow, &ux_hwm_info, &ux_menu_back_step, FLOW_LOOP);

void ui_settings(void) {
    hwm_status_to_string(home_context.hwm_status,
                         sizeof(home_context.hwm_status),
                         &g_hwm.hwm_disabled);
    ux_flow_init(0, ux_settings_flow, NULL);
}

void ui_toggle_hwm(void) {
    toggle_hwm();
    ui_settings();
}

tz_exc calculate_idle_screen_chain_id(void) {
    tz_exc exc = SW_OK;

    memset(&home_context.chain_id, 0, sizeof(home_context.chain_id));

    TZ_ASSERT(chain_id_to_string_with_aliases(home_context.chain_id,
                                              sizeof(home_context.chain_id),
                                              &g_hwm.main_chain_id) >= 0,
              EXC_WRONG_LENGTH);

end:
    return exc;
}

tz_exc calculate_idle_screen_authorized_key(void) {
    tz_exc exc = SW_OK;
    cx_err_t error = CX_OK;

    cx_ecfp_public_key_t *authorized_pk = (cx_ecfp_public_key_t *) &(tz_ecfp_public_key_t){0};

    memset(&home_context.authorized_key, 0, sizeof(home_context.authorized_key));

    if (g_hwm.baking_key.bip32_path.length == 0u) {
        TZ_ASSERT(copy_string(home_context.authorized_key,
                              sizeof(home_context.authorized_key),
                              "No Key Authorized"),
                  EXC_WRONG_LENGTH);
    } else {
        // Do not derive the public key if the two path_with_curve are equal
        if (!bip32_path_with_curve_eq(&global.path_with_curve, &g_hwm.baking_key)) {
            CX_CHECK(generate_public_key((cx_ecfp_public_key_t *) authorized_pk,
                                         &global.path_with_curve));
        } else {
            memmove(authorized_pk, &global.public_key, sizeof(tz_ecfp_public_key_t));
        }

        TZ_CHECK(pk_to_pkh_string(home_context.authorized_key,
                                  sizeof(home_context.authorized_key),
                                  authorized_pk));
    }

end:
    TZ_CONVERT_CX();
    return exc;
}

tz_exc calculate_idle_screen_hwm(void) {
    tz_exc exc = SW_OK;

    memset(&home_context.hwm, 0, sizeof(home_context.hwm));

    TZ_ASSERT(hwm_to_string(home_context.hwm, sizeof(home_context.hwm), &g_hwm.hwm.main) >= 0,
              EXC_WRONG_LENGTH);

end:
    return exc;
}

tz_exc calculate_baking_idle_screens_data(void) {
    tz_exc exc = SW_OK;

    TZ_CHECK(calculate_idle_screen_chain_id());

    TZ_CHECK(calculate_idle_screen_authorized_key());

    TZ_CHECK(calculate_idle_screen_hwm());

end:
    return exc;
}

void ui_initial_screen(void) {
    tz_exc exc = SW_OK;

    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    TZ_CHECK(calculate_baking_idle_screens_data());

    ui_menu_init();
    return;
end:
    TZ_EXC_PRINT(exc);
}

/**
 * @brief Refreshes the idle HWM screen
 *
 *        Used to update the HWM by pushing both buttons
 *
 */
static void ui_refresh_idle_hwm_screen(void) {
    ux_flow_init(0, ux_idle_flow, &ux_hwm_step);
}

void ux_prepare_confirm_callbacks(ui_callback_t ok_c, ui_callback_t cxl_c) {
    if (ok_c) {
        G_display.ok_callback = ok_c;
    }
    if (cxl_c) {
        G_display.cxl_callback = cxl_c;
    }
}

/**
 * @brief Callback called on accept or cancel
 *
 * @param accepted: true if accepted, false if cancelled
 */
static void prompt_response(bool const accepted) {
    if (accepted) {
        G_display.ok_callback();
    } else {
        G_display.cxl_callback();
    }
    ui_initial_screen();
}

UX_STEP_CB(ux_prompt_flow_reject_step, pb, prompt_response(false), {&C_icon_crossmark, "Reject"});
UX_STEP_CB(ux_prompt_flow_accept_step, pb, prompt_response(true), {&C_icon_validate_14, "Accept"});
UX_STEP_NOCB(ux_eye_step, nn, {"Review", "Request"});

void refresh_screens(void) {
    ux_stack_redisplay();
}

#endif  // HAVE_BAGL
