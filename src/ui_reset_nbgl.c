/* Tezos Ledger application - Reset BAGL UI handling

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

#include "ui_reset.h"

#include "apdu_reset.h"
#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#include "nbgl_use_case.h"
#define G global.apdu.u.baking

/**
 * @brief This structure represents a context needed for reset screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[1];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    char buffer[MAX_INT_DIGITS + 1u];  /// value buffer
} ResetContext_t;

/// Current reset context
static ResetContext_t reset_context;

/**
 * @brief Callback called when reset is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        reset_context.ok_cb();
        nbgl_useCaseStatus("RESET\nCONFIRMED", true, ui_initial_screen);
    } else {
        reset_context.cxl_cb();
        nbgl_useCaseStatus("Reset cancelled", false, ui_initial_screen);
    }
}

/**
 * @brief Callback called when reset is cancelled
 *
 */
static void cancel_callback(void) {
    confirmation_callback(false);
}

/**
 * @brief Draws a confirmation page
 *
 */
static void confirm_reset_page(void) {
    reset_context.tagValueList.pairs = reset_context.tagValuePair;

    reset_context.infoLongPress.icon = &C_tezos;
    reset_context.infoLongPress.longPressText = "Approve";
    reset_context.infoLongPress.tuneId = TUNE_TAP_CASUAL;
    reset_context.infoLongPress.text = "Confirm HWM reset";

    nbgl_useCaseStaticReviewLight(&reset_context.tagValueList,
                                  &reset_context.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

int prompt_reset(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    reset_context.ok_cb = ok_cb;
    reset_context.cxl_cb = cxl_cb;

    if (number_to_string(reset_context.buffer, sizeof(reset_context.buffer), G.reset_level) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    reset_context.tagValuePair[0].item = "Reset level";
    reset_context.tagValuePair[0].value = reset_context.buffer;

    reset_context.tagValueList.nbPairs = 1;

    nbgl_useCaseReviewStart(&C_tezos,
                            "Reset HWM",
                            NULL,
                            "Cancel",
                            confirm_reset_page,
                            cancel_callback);
    return 0;
}

#endif  // HAVE_NBGL
