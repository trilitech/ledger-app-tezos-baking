/* Tezos Ledger application - Baking APDU instruction handling

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

#include "apdu_baking.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.baking

size_t handle_apdu_reset(__attribute__((unused)) uint8_t instruction, volatile uint32_t* flags) {
    uint8_t* dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;
    uint32_t dataLength = G_io_apdu_buffer[OFFSET_LC];
    if (dataLength != sizeof(level_t)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }
    level_t const lvl = READ_UNALIGNED_BIG_ENDIAN(level_t, dataBuffer);
    if (!is_valid_level(lvl)) {
        THROW(EXC_PARSE_ERROR);
    }

    G.reset_level = lvl;
    ui_baking_reset(flags);
    return 0;
}

bool reset_ok(void) {
    UPDATE_NVRAM(ram, {
        ram->hwm.main.highest_level = G.reset_level;
        ram->hwm.main.highest_round = 0;
        ram->hwm.main.had_endorsement = false;
        ram->hwm.test.highest_level = G.reset_level;
        ram->hwm.test.highest_round = 0;
        ram->hwm.test.had_endorsement = false;
    });

    // Send back the response, do not restart the event loop
    delayed_send(finalize_successful_send(0));
    return true;
}

size_t send_word_big_endian(size_t tx, uint32_t word) {
    char word_bytes[sizeof(word)];

    memcpy(word_bytes, &word, sizeof(word));

    // endian.h functions do not compile
    uint32_t i = 0;
    for (; i < sizeof(word); i++) {
        G_io_apdu_buffer[i + tx] = word_bytes[sizeof(word) - i - 1];
    }

    return tx + i;
}

size_t handle_apdu_all_hwm(__attribute__((unused)) uint8_t instruction,
                           __attribute__((unused)) volatile uint32_t* flags) {
    size_t tx = 0;
    tx = send_word_big_endian(tx, N_data.hwm.main.highest_level);
    int has_a_chain_migrated =
        N_data.hwm.main.migrated_to_tenderbake || N_data.hwm.test.migrated_to_tenderbake;
    if (has_a_chain_migrated) {
        tx = send_word_big_endian(tx, N_data.hwm.main.highest_round);
    }
    tx = send_word_big_endian(tx, N_data.hwm.test.highest_level);
    if (has_a_chain_migrated) {
        tx = send_word_big_endian(tx, N_data.hwm.test.highest_round);
    }
    tx = send_word_big_endian(tx, N_data.main_chain_id.v);
    return finalize_successful_send(tx);
}

size_t handle_apdu_main_hwm(__attribute__((unused)) uint8_t instruction,
                            __attribute__((unused)) volatile uint32_t* flags) {
    size_t tx = 0;
    tx = send_word_big_endian(tx, N_data.hwm.main.highest_level);
    if (N_data.hwm.main.migrated_to_tenderbake) {
        tx = send_word_big_endian(tx, N_data.hwm.main.highest_round);
    }
    return finalize_successful_send(tx);
}

size_t handle_apdu_query_auth_key(__attribute__((unused)) uint8_t instruction,
                                  __attribute__((unused)) volatile uint32_t* flags) {
    uint8_t const length = N_data.baking_key.bip32_path.length;

    size_t tx = 0;
    G_io_apdu_buffer[tx++] = length;

    for (uint8_t i = 0; i < length; ++i) {
        tx = send_word_big_endian(tx, N_data.baking_key.bip32_path.components[i]);
    }

    return finalize_successful_send(tx);
}

size_t handle_apdu_query_auth_key_with_curve(__attribute__((unused)) uint8_t instruction,
                                             __attribute__((unused)) volatile uint32_t* flags) {
    uint8_t const length = N_data.baking_key.bip32_path.length;

    size_t tx = 0;
    G_io_apdu_buffer[tx++] = unparse_derivation_type(N_data.baking_key.derivation_type);
    G_io_apdu_buffer[tx++] = length;
    for (uint8_t i = 0; i < length; ++i) {
        tx = send_word_big_endian(tx, N_data.baking_key.bip32_path.components[i]);
    }

    return finalize_successful_send(tx);
}

size_t handle_apdu_deauthorize(__attribute__((unused)) uint8_t instruction,
                               __attribute__((unused)) volatile uint32_t* flags) {
    if (G_io_apdu_buffer[OFFSET_P1] != 0) {
        THROW(EXC_WRONG_PARAM);
    }
    if (G_io_apdu_buffer[OFFSET_LC] != 0) {
        THROW(EXC_PARSE_ERROR);
    }
    UPDATE_NVRAM(ram, { memset(&ram->baking_key, 0, sizeof(ram->baking_key)); });

    return finalize_successful_send(0);
}
