/* Tezos Ledger application - Setup NBGL UI handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger

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

#ifdef HAVE_NBGL

#include "nbgl_use_case.h"
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

#define MAX_LENGTH 100

/**
 * @brief This structure represents a context needed for setup screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[4];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    char buffer[4][MAX_LENGTH];  /// values buffers
} SetupContext_t;

/// Current setup context
static SetupContext_t setup_context;

/**
 * @brief Callback called when setup is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        setup_context.ok_cb();
        nbgl_useCaseStatus("SETUP\nCONFIRMED", true, ui_initial_screen);
    } else {
        setup_context.cxl_cb();
        nbgl_useCaseStatus("Setup cancelled", false, ui_initial_screen);
    }
}

/**
 * @brief Callback called when setup is cancelled
 *
 */
static void cancel_callback(void) {
    confirmation_callback(false);
}

/**
 * @brief Draws a confirmation page
 *
 */
static void confirm_setup_page(void) {
    setup_context.infoLongPress.icon = &C_tezos;
    setup_context.infoLongPress.longPressText = "Approve";
    setup_context.infoLongPress.tuneId = TUNE_TAP_CASUAL;
    setup_context.infoLongPress.text = "Confirm baking setup";

    nbgl_useCaseStaticReviewLight(&setup_context.tagValueList,
                                  &setup_context.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

int prompt_setup(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    setup_context.ok_cb = ok_cb;
    setup_context.cxl_cb = cxl_cb;

    bip32_path_with_curve_to_pkh_string(setup_context.buffer[0],
                                        sizeof(setup_context.buffer[0]),
                                        &global.path_with_curve);
    chain_id_to_string_with_aliases(setup_context.buffer[1],
                                    sizeof(setup_context.buffer[1]),
                                    &G.main_chain_id);

    number_to_string_indirect32(setup_context.buffer[2],
                                sizeof(setup_context.buffer[2]),
                                &G.hwm.main);

    number_to_string_indirect32(setup_context.buffer[3],
                                sizeof(setup_context.buffer[3]),
                                &G.hwm.test);

    setup_context.tagValuePair[0].item = "Address";
    setup_context.tagValuePair[0].value = setup_context.buffer[0];

    setup_context.tagValuePair[1].item = "Chain";
    setup_context.tagValuePair[1].value = setup_context.buffer[1];

    setup_context.tagValuePair[2].item = "Main Chain HWM";
    setup_context.tagValuePair[2].value = setup_context.buffer[2];

    setup_context.tagValuePair[3].item = "Test Chain HWM";
    setup_context.tagValuePair[3].value = setup_context.buffer[3];

    setup_context.tagValueList.nbPairs = 4;
    setup_context.tagValueList.pairs = setup_context.tagValuePair;

    setup_context.infoLongPress.text = "Confirm baking setup";

    nbgl_useCaseReviewStart(&C_tezos,
                            "Setup baking",
                            NULL,
                            "Cancel",
                            confirm_setup_page,
                            cancel_callback);
    return 0;
}

#endif  // HAVE_NBGL
