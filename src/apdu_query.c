/* Tezos Ledger application - Query APDU instruction handling

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

#include "apdu_query.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

/**
 * @brief Inserts big endian word in the apdu response
 *
 * @param tx: current offset of the apdu response
 * @param word: big endian word
 * @return size_t: updated offset of the apdu response
 */
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
