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
    struct bip32_path_wire bip32_path;  ///< authorized key path
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
    delayed_send(provide_pubkey(G_io_apdu_buffer, &pubkey));
    return true;
}

size_t handle_apdu_setup(const command_t *cmd, volatile uint32_t *flags) {
    check_null(cmd);

    if (cmd->p1 != 0u) {
        THROW(EXC_WRONG_PARAM);
    }

    if (cmd->lc < sizeof(struct setup_wire)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    global.path_with_curve.derivation_type = parse_derivation_type(cmd->p2);

    {
        struct setup_wire const *const buff_as_setup = (struct setup_wire const *) cmd->data;

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
        consumed += read_bip32_path(&global.path_with_curve.bip32_path,
                                    (uint8_t const *) &buff_as_setup->bip32_path,
                                    cmd->lc - consumed);

        if (consumed != cmd->lc) {
            THROW(EXC_WRONG_LENGTH);
        }
    }

    prompt_setup(ok, delay_reject);
    *flags = IO_ASYNCH_REPLY;
    return 0;
}

size_t handle_apdu_deauthorize(const command_t *cmd) {
    check_null(cmd);

    if (cmd->p1 != 0u) {
        THROW(EXC_WRONG_PARAM);
    }
    if (cmd->lc != 0u) {
        THROW(EXC_PARSE_ERROR);
    }
    UPDATE_NVRAM(ram, { memset(&ram->baking_key, 0, sizeof(ram->baking_key)); });
#ifdef HAVE_BAGL
    update_baking_idle_screens();
#endif  // HAVE_BAGL

    return finalize_successful_send(0);
}
