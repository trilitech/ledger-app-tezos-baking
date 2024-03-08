/* Tezos Ledger application - Public key NBGL UI handling

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
#include "apdu_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "baking_auth.h"

#include <string.h>

#define G global.apdu.u.baking

typedef struct {
    char buffer[sizeof(cx_ecfp_public_key_t)];
    ui_callback_t ok_cb;
    ui_callback_t cxl_cb;
} TransactionContext_t;

static TransactionContext_t transactionContext;

static void cancel_callback(void) {
    transactionContext.cxl_cb();
    nbgl_useCaseStatus("Address rejected", false, ui_initial_screen);
}

static void approve_callback(void) {
    transactionContext.ok_cb();
    nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, ui_initial_screen);
}

static void confirmation_callback(bool confirm) {
    if (confirm) {
        approve_callback();
    } else {
        cancel_callback();
    }
}

static void verify_address(void) {
    nbgl_useCaseAddressConfirmation(transactionContext.buffer, confirmation_callback);
}

void prompt_address(bool baking, ui_callback_t ok_cb, ui_callback_t cxl_cb) {
    transactionContext.ok_cb = ok_cb;
    transactionContext.cxl_cb = cxl_cb;

    bip32_path_with_curve_to_pkh_string(transactionContext.buffer,
                                        sizeof(transactionContext.buffer),
                                        &global.path_with_curve);

    const char* text;
    if (baking) {
        text = "Authorize Tezos\nBaking address";
    } else {
        text = "Verify Tezos\naddress";
    }
    nbgl_useCaseReviewStart(&C_tezos, text, NULL, "Cancel", verify_address, cancel_callback);
}
#endif  // HAVE_NBGL
