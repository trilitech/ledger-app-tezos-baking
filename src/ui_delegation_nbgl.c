/* Tezos Ledger application - Delegate NBGL UI handling

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
#include "apdu_sign.h"
#include "ui_delegation.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "to_string.h"
#include "ui.h"
#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define MAX_LENGTH 100

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

/**
 * @brief This structure represents a context needed for delegation screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[6];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    const char* confirmed_status;
    const char* cancelled_status;
    char buffer[6][MAX_LENGTH];
} DelegationContext_t;

/// Current delegation context
static DelegationContext_t delegation_context;

/**
 * @brief Callback called when delegation is rejected
 *
 */
static void cancel_callback(void) {
    nbgl_useCaseStatus(delegation_context.cancelled_status, false, ui_initial_screen);
    delegation_context.cxl_cb();
}

/**
 * @brief Callback called when delegation is approved
 *
 */
static void approve_callback(void) {
    nbgl_useCaseStatus(delegation_context.confirmed_status, true, ui_initial_screen);
    delegation_context.ok_cb();
}

/**
 * @brief Callback called when delegation is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        approve_callback();
    } else {
        cancel_callback();
    }
}

/**
 * @brief Draws a confirmation page
 *
 */
static void confirm_delegation_page(void) {
    delegation_context.infoLongPress.icon = &C_tezos;
    delegation_context.infoLongPress.longPressText = "Approve";
    delegation_context.infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReviewLight(&delegation_context.tagValueList,
                                  &delegation_context.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

int prompt_delegation(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    if (!G.maybe_ops.is_valid) {
        THROW(EXC_MEMORY_ERROR);
    }

    delegation_context.ok_cb = ok_cb;
    delegation_context.cxl_cb = cxl_cb;

    tz_exc exc = bip32_path_with_curve_to_pkh_string(delegation_context.buffer[0],
                                                     sizeof(delegation_context.buffer[0]),
                                                     &global.path_with_curve);

    if (exc != SW_OK) {
        THROW(exc);
    }

    if (microtez_to_string(delegation_context.buffer[1],
                           sizeof(delegation_context.buffer[1]),
                           G.maybe_ops.v.total_fee) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    delegation_context.tagValuePair[0].item = "Address";
    delegation_context.tagValuePair[0].value = delegation_context.buffer[0];

    delegation_context.tagValuePair[1].item = "Fee";
    delegation_context.tagValuePair[1].value = delegation_context.buffer[1];

    delegation_context.tagValueList.nbPairs = 2;
    delegation_context.tagValueList.pairs = delegation_context.tagValuePair;

    delegation_context.infoLongPress.text = "Confirm delegate\nregistration";

    delegation_context.confirmed_status = "DELEGATE\nCONFIRMED";
    delegation_context.cancelled_status = "Delegate registration\ncancelled";

    nbgl_useCaseReviewStart(&C_tezos,
                            "Register delegate",
                            NULL,
                            "Cancel",
                            confirm_delegation_page,
                            cancel_callback);
    return 0;
}

#endif  // HAVE_NBGL
