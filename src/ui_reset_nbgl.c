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
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#include "nbgl_use_case.h"
#define G global.apdu.u.baking

typedef struct {
    ui_callback_t ok_cb;
    ui_callback_t cxl_cb;
    nbgl_layoutTagValue_t tagValuePair[1];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    char buffer[MAX_INT_DIGITS + 1];
} TransactionContext_t;

static TransactionContext_t transactionContext;

static void confirmation_callback(bool confirm) {
    if (confirm) {
        transactionContext.ok_cb();
        nbgl_useCaseStatus("RESET\nCONFIRMED", true, ui_initial_screen);
    } else {
        transactionContext.cxl_cb();
        nbgl_useCaseStatus("Reset cancelled", false, ui_initial_screen);
    }
}

static void cancel_callback(void) {
    confirmation_callback(false);
}

static void continue_light_callback(void) {
    transactionContext.tagValueList.pairs = transactionContext.tagValuePair;

    transactionContext.infoLongPress.icon = &C_tezos;
    transactionContext.infoLongPress.longPressText = "Approve";
    transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;
    transactionContext.infoLongPress.text = "Confirm HWM reset";

    nbgl_useCaseStaticReviewLight(&transactionContext.tagValueList,
                                  &transactionContext.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

void ui_baking_reset(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    transactionContext.ok_cb = ok_cb;
    transactionContext.cxl_cb = cxl_cb;

    number_to_string_indirect32(transactionContext.buffer,
                                sizeof(transactionContext.buffer),
                                &G.reset_level);

    transactionContext.tagValuePair[0].item = "Reset level";
    transactionContext.tagValuePair[0].value = transactionContext.buffer;

    transactionContext.tagValueList.nbPairs = 1;

    nbgl_useCaseReviewStart(&C_tezos,
                            "Reset HWM",
                            NULL,
                            "Cancel",
                            continue_light_callback,
                            cancel_callback);
}

#endif  // HAVE_NBGL
