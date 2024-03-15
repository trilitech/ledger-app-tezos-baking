/* Tezos Ledger application - Common apdu primitives

   Copyright 2023 Ledger
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

#include "apdu.h"
#include "globals.h"
#include "to_string.h"
#include "version.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

size_t provide_pubkey(uint8_t* const io_buffer, cx_ecfp_public_key_t const* const pubkey) {
    check_null(io_buffer);
    check_null(pubkey);
    size_t tx = 0;
    // Application could be PIN-locked, and pubkey->W_len would then be 0,
    // so throwing an error rather than returning an empty key
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        THROW(EXC_SECURITY);
    }
    io_buffer[tx] = pubkey->W_len;
    tx++;
    memmove(io_buffer + tx, pubkey->W, pubkey->W_len);
    tx += pubkey->W_len;
    return finalize_successful_send(tx);
}

size_t handle_apdu_error(uint8_t __attribute__((unused)) instruction,
                         volatile uint32_t* __attribute__((unused)) flags) {
    THROW(EXC_INVALID_INS);
}

size_t handle_apdu_version(uint8_t __attribute__((unused)) instruction,
                           volatile uint32_t* __attribute__((unused)) flags) {
    memcpy(G_io_apdu_buffer, &version, sizeof(version_t));
    size_t tx = sizeof(version_t);
    return finalize_successful_send(tx);
}

size_t handle_apdu_git(uint8_t __attribute__((unused)) instruction,
                       volatile uint32_t* __attribute__((unused)) flags) {
    static const char commit[] = COMMIT;
    memcpy(G_io_apdu_buffer, commit, sizeof(commit));
    size_t tx = sizeof(commit);
    return finalize_successful_send(tx);
}

#define CLA 0x80  /// The only APDU class that will be used

size_t apdu_dispatcher(volatile uint32_t* flags) {
    if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(EXC_CLASS);
    }

    uint8_t const instruction = G_io_apdu_buffer[OFFSET_INS];
    apdu_handler const cb =
        (instruction >= (INS_MAX + 1u)) ? handle_apdu_error : global.handlers[instruction];

    return cb(instruction, flags);
}
