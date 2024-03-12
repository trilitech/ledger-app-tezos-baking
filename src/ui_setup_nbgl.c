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

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.setup

#define MAX_LENGTH 100

typedef struct {
    ui_callback_t ok_cb;
    ui_callback_t cxl_cb;
    nbgl_layoutTagValue_t tagValuePair[4];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    char buffer[4][MAX_LENGTH];
} TransactionContext_t;

static TransactionContext_t transactionContext;

static void confirmation_callback(bool confirm) {
    if (confirm) {
        transactionContext.ok_cb();
        nbgl_useCaseStatus("SETUP\nCONFIRMED", true, ui_initial_screen);
    } else {
        transactionContext.cxl_cb();
        nbgl_useCaseStatus("Setup cancelled", false, ui_initial_screen);
    }
}

static void cancel_callback(void) {
    confirmation_callback(false);
}

static void continue_light_callback(void) {
    transactionContext.infoLongPress.icon = &C_tezos;
    transactionContext.infoLongPress.longPressText = "Approve";
    transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;
    transactionContext.infoLongPress.text = "Confirm baking setup";

    nbgl_useCaseStaticReviewLight(&transactionContext.tagValueList,
                                  &transactionContext.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

void prompt_setup(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    transactionContext.ok_cb = ok_cb;
    transactionContext.cxl_cb = cxl_cb;

    bip32_path_with_curve_to_pkh_string(transactionContext.buffer[0],
                                        sizeof(transactionContext.buffer[0]),
                                        &global.path_with_curve);
    chain_id_to_string_with_aliases(transactionContext.buffer[1],
                                    sizeof(transactionContext.buffer[1]),
                                    &G.main_chain_id);

    number_to_string_indirect32(transactionContext.buffer[2],
                                sizeof(transactionContext.buffer[2]),
                                &G.hwm.main);

    number_to_string_indirect32(transactionContext.buffer[3],
                                sizeof(transactionContext.buffer[3]),
                                &G.hwm.test);

    transactionContext.tagValuePair[0].item = "Address";
    transactionContext.tagValuePair[0].value = transactionContext.buffer[0];

    transactionContext.tagValuePair[1].item = "Chain";
    transactionContext.tagValuePair[1].value = transactionContext.buffer[1];

    transactionContext.tagValuePair[2].item = "Main Chain HWM";
    transactionContext.tagValuePair[2].value = transactionContext.buffer[2];

    transactionContext.tagValuePair[3].item = "Test Chain HWM";
    transactionContext.tagValuePair[3].value = transactionContext.buffer[3];

    transactionContext.tagValueList.nbPairs = 4;
    transactionContext.tagValueList.pairs = transactionContext.tagValuePair;

    transactionContext.infoLongPress.text = "Confirm baking setup";

    nbgl_useCaseReviewStart(&C_tezos,
                            "Setup baking",
                            NULL,
                            "Cancel",
                            continue_light_callback,
                            cancel_callback);
}

#endif  // HAVE_NBGL
