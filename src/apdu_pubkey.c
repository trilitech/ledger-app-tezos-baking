/* Tezos Ledger application - Public key APDU instruction handling

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

#include "apdu_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"
#include "ui_pubkey.h"
#include "baking_auth.h"

#include <string.h>

/**
 * @brief Sends apdu response with the public key
 *
 * @return true
 */
static bool pubkey_ok(void) {
    provide_pubkey(&global.path_with_curve);
    return true;
}

/**
 * @brief Authorizes the public key
 *
 *        Sends apdu response with the public key
 *
 * @return true
 */
static bool baking_ok(void) {
    tz_exc exc = SW_OK;

    TZ_CHECK(authorize_baking(global.path_with_curve.derivation_type,
                              &global.path_with_curve.bip32_path));
    return pubkey_ok();

end:
    return io_send_apdu_err(exc);
}

/**
 * Cdata:
 *   + Bip32 path: public key path
 */
int handle_get_public_key(buffer_t *cdata,
                          derivation_type_t derivation_type,
                          bool authorize,
                          bool prompt) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(cdata);

    global.path_with_curve.derivation_type = derivation_type;
    if ((cdata->size == 0u) && authorize) {
        TZ_ASSERT(copy_bip32_path_with_curve(&global.path_with_curve, &(g_hwm.baking_key)),
                  EXC_MEMORY_ERROR);
    } else {
        TZ_ASSERT(read_bip32_path(cdata, &global.path_with_curve.bip32_path), EXC_WRONG_VALUES);
    }

    TZ_ASSERT(cdata->size == cdata->offset, EXC_WRONG_LENGTH);

    if (!prompt) {
        return provide_pubkey(&global.path_with_curve);
    } else {
        // INS_PROMPT_PUBLIC_KEY || INS_AUTHORIZE_BAKING
        ui_callback_t cb;
        bool bake;
        if (authorize) {
            cb = baking_ok;
            bake = true;
        } else {
            // INS_PROMPT_PUBLIC_KEY
            cb = pubkey_ok;
            bake = false;
        }
        return prompt_pubkey(bake, cb, reject);
    }

end:
    return io_send_apdu_err(exc);
}
