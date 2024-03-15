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

#include "apdu_hmac.h"
#include "apdu_pubkey.h"
#include "apdu_query.h"
#include "apdu_reset.h"
#include "apdu_setup.h"
#include "apdu_sign.h"
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

/**
 * @brief Handles VERSION instruction
 *
 *        Fills apdu response with the app version
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
static size_t handle_apdu_version(uint8_t __attribute__((unused)) instruction,
                                  volatile uint32_t* __attribute__((unused)) flags) {
    memcpy(G_io_apdu_buffer, &version, sizeof(version_t));
    size_t tx = sizeof(version_t);
    return finalize_successful_send(tx);
}

/**
 * @brief Handles GIT instruction
 *
 *        Fills apdu response with the app commit
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
static size_t handle_apdu_git(uint8_t __attribute__((unused)) instruction,
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

    int result = 0;
    uint8_t const instruction = G_io_apdu_buffer[OFFSET_INS];
    switch (instruction) {
        case INS_VERSION:
            result = handle_apdu_version(instruction, flags);
            break;
        case INS_GET_PUBLIC_KEY:
            result = handle_apdu_get_public_key(instruction, flags);
            break;
        case INS_PROMPT_PUBLIC_KEY:
            result = handle_apdu_get_public_key(instruction, flags);
            break;
        case INS_SIGN:
            result = handle_apdu_sign(instruction, flags);
            break;
        case INS_GIT:
            result = handle_apdu_git(instruction, flags);
            break;
        case INS_SIGN_WITH_HASH:
            result = handle_apdu_sign_with_hash(instruction, flags);
            break;
        case INS_AUTHORIZE_BAKING:
            result = handle_apdu_get_public_key(instruction, flags);
            break;
        case INS_RESET:
            result = handle_apdu_reset(instruction, flags);
            break;
        case INS_QUERY_AUTH_KEY:
            result = handle_apdu_query_auth_key(instruction, flags);
            break;
        case INS_QUERY_MAIN_HWM:
            result = handle_apdu_main_hwm(instruction, flags);
            break;
        case INS_SETUP:
            result = handle_apdu_setup(instruction, flags);
            break;
        case INS_QUERY_ALL_HWM:
            result = handle_apdu_all_hwm(instruction, flags);
            break;
        case INS_DEAUTHORIZE:
            result = handle_apdu_deauthorize(instruction, flags);
            break;
        case INS_QUERY_AUTH_KEY_WITH_CURVE:
            result = handle_apdu_query_auth_key_with_curve(instruction, flags);
            break;
        case INS_HMAC:
            result = handle_apdu_hmac(instruction, flags);
            break;
        default:
            THROW(EXC_INVALID_INS);
    }
    return result;
}
