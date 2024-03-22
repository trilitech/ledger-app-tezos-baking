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

#include <stdbool.h>
#include <string.h>

#define G_display global.dynamic_display

/**
 * @brief This structure represents a context needed for home screens navigation
 *
 */
typedef struct {
    char chain_id[CHAIN_ID_BASE58_STRING_SIZE];
    char authorized_key[PKH_STRING_SIZE];
    char hwm[MAX_INT_DIGITS + 1u + MAX_INT_DIGITS + 1u];
} HomeContext_t;

/// Current home context
static HomeContext_t home_context;

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
UX_STEP_NOCB(ux_app_is_ready_step, nn, {"Application", "is ready"});
UX_STEP_NOCB(ux_version_step, bnnn_paging, {"Tezos Baking", VERSION});
UX_STEP_NOCB(ux_chain_id_step, bnnn_paging, {"Chain", home_context.chain_id});
UX_STEP_NOCB(ux_authorized_key_step, bnnn_paging, {"Public Key Hash", home_context.authorized_key});
UX_STEP_NOCB(ux_hwm_step, bnnn_paging, {"High Watermark", home_context.hwm});
UX_STEP_CB(ux_idle_quit_step, pb, exit_app(), {&C_icon_dashboard_x, "Quit"});

UX_FLOW(ux_idle_flow,
        &ux_app_is_ready_step,
        &ux_version_step,
        &ux_chain_id_step,
        &ux_authorized_key_step,
        &ux_hwm_step,
        &ux_idle_quit_step,
        FLOW_LOOP);

/**
 * @brief Calculates baking values for the idle screens
 *
 */
static void calculate_baking_idle_screens_data(void) {
    memset(&home_context, 0, sizeof(home_context));

    if (chain_id_to_string_with_aliases(home_context.chain_id,
                                        sizeof(home_context.chain_id),
                                        &N_data.main_chain_id) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    if (N_data.baking_key.bip32_path.length == 0u) {
        if (!copy_string(home_context.authorized_key,
                         sizeof(home_context.authorized_key),
                         "No Key Authorized")) {
            THROW(EXC_WRONG_LENGTH);
        }
    } else {
        tz_exc exc = bip32_path_with_curve_to_pkh_string(home_context.authorized_key,
                                                         sizeof(home_context.authorized_key),
                                                         &N_data.baking_key);
        if (exc != SW_OK) {
            THROW(exc);
        }
    }

    if (hwm_to_string(home_context.hwm, sizeof(home_context.hwm), &N_data.hwm.main) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }
}

void ui_initial_screen(void) {
    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    calculate_baking_idle_screens_data();

    ux_flow_init(0, ux_idle_flow, NULL);
}

void update_baking_idle_screens(void) {
    calculate_baking_idle_screens_data();
    /// refresh
    ux_stack_display(0);
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

#endif  // HAVE_BAGL
