/* Tezos Ledger application - Setup BAGL UI handling

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

#include "apdu_setup.h"
#include "ui_setup.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.setup

/**
 * @brief This structure represents a context needed for setup screens navigation
 *
 */
typedef struct {
    char address[PKH_STRING_SIZE];
    char chain[CHAIN_ID_BASE58_STRING_SIZE];
    char main_hwm[MAX_INT_DIGITS + 1u];
    char test_hwm[MAX_INT_DIGITS + 1u];
} SetupContext_t;

/// Current setup context
static SetupContext_t setup_context;

UX_STEP_NOCB(ux_setup_step, bnnn_paging, {"Setup", "Baking?"});
UX_STEP_NOCB(ux_address_step, bnnn_paging, {"Address", setup_context.address});
UX_STEP_NOCB(ux_chain_step, bnnn_paging, {"Chain", setup_context.chain});
UX_STEP_NOCB(ux_main_hwm_step, bnnn_paging, {"Main Chain HWM", setup_context.main_hwm});
UX_STEP_NOCB(ux_test_hwm_step, bnnn_paging, {"Test Chain HWM", setup_context.test_hwm});

UX_CONFIRM_FLOW(ux_setup_flow,
                &ux_setup_step,
                &ux_address_step,
                &ux_chain_step,
                &ux_main_hwm_step,
                &ux_test_hwm_step);

int prompt_setup(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    memset(&setup_context, 0, sizeof(setup_context));

    tz_exc exc = bip32_path_with_curve_to_pkh_string(setup_context.address,
                                                     sizeof(setup_context.address),
                                                     &global.path_with_curve);

    if (exc != SW_OK) {
        THROW(exc);
    }

    if (chain_id_to_string_with_aliases(setup_context.chain,
                                        sizeof(setup_context.chain),
                                        &G.main_chain_id) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    if (number_to_string(setup_context.main_hwm, sizeof(setup_context.main_hwm), G.hwm.main) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    if (number_to_string(setup_context.test_hwm, sizeof(setup_context.test_hwm), G.hwm.test) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    ux_prepare_confirm_callbacks(ok_cb, cxl_cb);
    ux_flow_init(0, ux_setup_flow, NULL);
    return 0;
}

#endif  // HAVE_BAGL
