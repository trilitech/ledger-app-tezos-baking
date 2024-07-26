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
#include "ui_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"
#include "baking_auth.h"

#include <string.h>

#define G global.apdu.u.baking

/**
 * @brief This structure represents a context needed for address screens navigation
 *
 */
typedef struct {
    char buffer[sizeof(cx_ecfp_public_key_t)];  /// value buffer
    ui_callback_t ok_cb;                        /// accept callback
    ui_callback_t cxl_cb;                       /// cancel callback
} AddressContext_t;

/// Current address context
static AddressContext_t address_context;

/**
 * @brief Callback called when address is approved or cancelled
 *
 * @param confirm: true if approved, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        address_context.ok_cb();
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_initial_screen);
    } else {
        address_context.cxl_cb();
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_initial_screen);
    }
}

int prompt_pubkey(bool authorize, ui_callback_t ok_cb, ui_callback_t cxl_cb) {
    tz_exc exc = SW_OK;

    address_context.ok_cb = ok_cb;
    address_context.cxl_cb = cxl_cb;

    TZ_CHECK(bip32_path_with_curve_to_pkh_string(address_context.buffer,
                                                 sizeof(address_context.buffer),
                                                 &global.path_with_curve));

    const char* text;
    if (authorize) {
        text = "Authorize Tezos\nBaking address";
    } else {
        text = "Verify Tezos\naddress";
    }
    nbgl_useCaseAddressReview(address_context.buffer,
                              NULL,
                              &C_tezos,
                              text,
                              NULL,
                              confirmation_callback);
    return 0;

end:
    return io_send_apdu_err(exc);
}

#endif  // HAVE_NBGL
