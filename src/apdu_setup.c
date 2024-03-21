/* Tezos Ledger application - Setup APDU instruction handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
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

#include "apdu_setup.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"
#include "ui_setup.h"

#include <string.h>

#define G global.apdu.u.setup

/**
 * @brief Applies the setup
 *
 *        Rounds are also reset to 0
 *
 * @return true
 */
static bool ok(void) {
    UPDATE_NVRAM(ram, {
        copy_bip32_path_with_curve(&ram->baking_key, &global.path_with_curve);
        ram->main_chain_id = G.main_chain_id;
        ram->hwm.main.highest_level = G.hwm.main;
        ram->hwm.main.highest_round = 0;
        ram->hwm.main.had_attestation = false;
        ram->hwm.main.had_preattestation = false;
        ram->hwm.main.migrated_to_tenderbake = false;
        ram->hwm.test.highest_level = G.hwm.test;
        ram->hwm.test.highest_round = 0;
        ram->hwm.test.had_attestation = false;
        ram->hwm.test.had_preattestation = false;
        ram->hwm.test.migrated_to_tenderbake = false;
    });

    cx_ecfp_public_key_t pubkey = {0};
    generate_public_key(&pubkey, &global.path_with_curve);
    provide_pubkey(&pubkey);
    return true;
}

/**
 * Cdata:
 *   + (4 bytes) uint32: chain id
 *   + (4 bytes) uint32: main hwm level
 *   + (4 bytes) uint32: test hwm level
 *   + Bip32 path: key path
 */
int handle_setup(buffer_t *cdata, derivation_type_t derivation_type) {
    check_null(cdata);

    global.path_with_curve.derivation_type = derivation_type;

    if (!buffer_read_u32(cdata, &G.main_chain_id.v, BE) ||           // chain id
        !buffer_read_u32(cdata, &G.hwm.main, BE) ||                  // main hwm level
        !buffer_read_u32(cdata, &G.hwm.test, BE) ||                  // test hwm level
        !read_bip32_path(cdata, &global.path_with_curve.bip32_path)  // key path
    ) {
        THROW(EXC_WRONG_VALUES);
    }

    if (cdata->size != cdata->offset) {
        THROW(EXC_WRONG_LENGTH);
    }

    return prompt_setup(ok, reject);
}

int handle_deauthorize(void) {
    UPDATE_NVRAM(ram, { memset(&ram->baking_key, 0, sizeof(ram->baking_key)); });
#ifdef HAVE_BAGL
    update_baking_idle_screens();
#endif  // HAVE_BAGL

    return io_send_sw(SW_OK);
}
