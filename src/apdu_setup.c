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
 * @brief This structure represents the SETUP instruction payload
 *
 */
struct setup_wire {
    uint32_t main_chain_id;  ///< main chain id
    /// high watermarks
    struct {
        uint32_t main;  ///< main highest level
        uint32_t test;  ///< test highest level
    } hwm;
} __attribute__((packed));

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
    generate_public_key(&pubkey,
                        global.path_with_curve.derivation_type,
                        &global.path_with_curve.bip32_path);
    provide_pubkey(&pubkey);
    return true;
}

int handle_setup(buffer_t *cdata, derivation_type_t derivation_type) {
    check_null(cdata);

    if (cdata->size < sizeof(struct setup_wire)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    global.path_with_curve.derivation_type = derivation_type;

    {
        struct setup_wire const *const buff_as_setup = (struct setup_wire const *) cdata->ptr;

        size_t consumed = 0;
        G.main_chain_id.v =
            CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                         uint32_t,
                                         (uint8_t const *) &buff_as_setup->main_chain_id);
        G.hwm.main = CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                                  uint32_t,
                                                  (uint8_t const *) &buff_as_setup->hwm.main);
        G.hwm.test = CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                                  uint32_t,
                                                  (uint8_t const *) &buff_as_setup->hwm.test);

        buffer_seek_set(cdata, consumed);

        if (!read_bip32_path(cdata, &global.path_with_curve.bip32_path)) {
            THROW(EXC_WRONG_VALUES);
        }

        if (cdata->size != cdata->offset) {
            THROW(EXC_WRONG_LENGTH);
        }
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
