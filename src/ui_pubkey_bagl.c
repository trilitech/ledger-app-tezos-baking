/* Tezos Ledger application - Public key BAGL UI handling

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
#include "apdu_pubkey.h"
#include "ui_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "baking_auth.h"

#include <string.h>

/**
 * @brief This structure represents a context needed for address screens navigation
 *
 */
typedef struct {
    char public_key_hash[PKH_STRING_SIZE];
} AddressContext_t;

/// Current address context
static AddressContext_t address_context;

UX_STEP_NOCB(ux_authorize_step, bnnn_paging, {"Authorize Baking", "With Public Key?"});
UX_STEP_NOCB(ux_provide_step, bnnn_paging, {"Provide", "Public Key"});
UX_STEP_NOCB(ux_public_key_hash_step,
             bnnn_paging,
             {"Public Key Hash", address_context.public_key_hash});

UX_CONFIRM_FLOW(ux_authorize_flow, &ux_authorize_step, &ux_public_key_hash_step);
UX_CONFIRM_FLOW(ux_provide_flow, &ux_provide_step, &ux_public_key_hash_step);

int prompt_pubkey(bool authorize, ui_callback_t ok_cb, ui_callback_t cxl_cb) {
    memset(&address_context, 0, sizeof(address_context));

    bip32_path_with_curve_to_pkh_string(address_context.public_key_hash,
                                        sizeof(address_context.public_key_hash),
                                        &global.path_with_curve);

    ux_prepare_confirm_callbacks(ok_cb, cxl_cb);
    if (authorize) {
        ux_flow_init(0, ux_authorize_flow, NULL);
    } else {
        ux_flow_init(0, ux_provide_flow, NULL);
    }
    return 0;
}

#endif  // HAVE_BAGL
