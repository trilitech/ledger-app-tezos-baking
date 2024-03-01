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
#include "protocol.h"
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
    cx_ecfp_public_key_t public_key = {0};
    generate_public_key(&public_key,
                        global.path_with_curve.derivation_type,
                        &global.path_with_curve.bip32_path);
    delayed_send(provide_pubkey(G_io_apdu_buffer, &public_key));
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
    authorize_baking(global.path_with_curve.derivation_type, &global.path_with_curve.bip32_path);
    pubkey_ok();
    return true;
}

size_t handle_apdu_get_public_key(uint8_t instruction, volatile uint32_t *flags) {
    uint8_t *dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;

    if (G_io_apdu_buffer[OFFSET_P1] != 0) {
        THROW(EXC_WRONG_PARAM);
    }

    global.path_with_curve.derivation_type = parse_derivation_type(G_io_apdu_buffer[OFFSET_CURVE]);

    size_t const cdata_size = G_io_apdu_buffer[OFFSET_LC];

    if (cdata_size == 0u && instruction == INS_AUTHORIZE_BAKING) {
        copy_bip32_path_with_curve(&global.path_with_curve, &N_data.baking_key);
    } else {
        read_bip32_path(&global.path_with_curve.bip32_path, dataBuffer, cdata_size);
        if (global.path_with_curve.bip32_path.length == 0u) {
            THROW(EXC_WRONG_LENGTH_FOR_INS);
        }
    }

    cx_ecfp_public_key_t public_key = {0};
    generate_public_key(&public_key,
                        global.path_with_curve.derivation_type,
                        &global.path_with_curve.bip32_path);

    if (instruction == INS_GET_PUBLIC_KEY) {
        return provide_pubkey(G_io_apdu_buffer, &public_key);
    } else {
        // instruction == INS_PROMPT_PUBLIC_KEY || instruction == INS_AUTHORIZE_BAKING
        ui_callback_t cb;
        bool bake;
        if (instruction == INS_AUTHORIZE_BAKING) {
            cb = baking_ok;
            bake = true;
        } else {
            // INS_PROMPT_PUBLIC_KEY
            cb = pubkey_ok;
            bake = false;
        }
        prompt_pubkey(bake, cb, delay_reject);
        *flags = IO_ASYNCH_REPLY;
        return 0;
    }
}
