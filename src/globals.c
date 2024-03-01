/* Tezos Ledger application - Global strcuture handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
   Copyright 2021 Ledger
   Copyright 2021 Obsidian Systems

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

#include "globals.h"

#include "exception.h"
#include "to_string.h"

#include "ux.h"

#include <string.h>

// WARNING: ***************************************************
// Non-const globals MUST NOT HAVE AN INITIALIZER.
//
// Providing an initializer will cause the application to crash
// if you write to it.
// ************************************************************

globals_t global;

void clear_apdu_globals(void) {
    memset(&global.apdu, 0, sizeof(global.apdu));
}

void init_globals(void) {
    memset(&global, 0, sizeof(global));
}

// DO NOT TRY TO INIT THIS. This can only be written via an system call.
// The "N_" is *significant*. It tells the linker to put this in NVRAM.
nvram_data const N_data_real;

high_watermark_t volatile *select_hwm_by_chain(chain_id_t const chain_id,
                                               nvram_data volatile *const ram) {
    check_null(ram);
    return chain_id.v == ram->main_chain_id.v || !ram->main_chain_id.v ? &ram->hwm.main
                                                                       : &ram->hwm.test;
}

void copy_chain(char *out, size_t out_size, chain_id_t *chain_id) {
    if (!chain_id->v) {
        copy_string(out, out_size, "any");
    } else {
        chain_id_to_string_with_aliases(out, out_size, (chain_id_t const *const) chain_id);
    }
}

void copy_key(char *out, size_t out_size, bip32_path_with_curve_t *baking_key) {
    if (baking_key->bip32_path.length == 0u) {
        copy_string(out, out_size, "No Key Authorized");
    } else {
        cx_ecfp_public_key_t pubkey = {0};
        generate_public_key(&pubkey,
                            (derivation_type_t const) baking_key->derivation_type,
                            (bip32_path_t const *const) &baking_key->bip32_path);
        pubkey_to_pkh_string(out,
                             out_size,
                             (derivation_type_t const) baking_key->derivation_type,
                             &pubkey);
    }
}

void copy_hwm(char *out, __attribute__((unused)) size_t out_size, high_watermark_t *hwm) {
    if (hwm->migrated_to_tenderbake) {
        size_t len1 = number_to_string(out, hwm->highest_level);
        out[len1] = ' ';
        number_to_string(out + len1 + 1u, hwm->highest_round);
    } else {
        number_to_string(out, hwm->highest_level);
    }
}
