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
 * @return size_t: offset of the apdu response
 */
static size_t handle_apdu_version(void) {
    memcpy(G_io_apdu_buffer, &version, sizeof(version_t));
    size_t tx = sizeof(version_t);
    return finalize_successful_send(tx);
}

/**
 * @brief Handles GIT instruction
 *
 *        Fills apdu response with the app commit
 *
 * @return size_t: offset of the apdu response
 */
static size_t handle_apdu_git(void) {
    static const char commit[] = COMMIT;
    memcpy(G_io_apdu_buffer, commit, sizeof(commit));
    size_t tx = sizeof(commit);
    return finalize_successful_send(tx);
}

#define CLA 0x80  /// The only APDU class that will be used

size_t apdu_dispatcher(const command_t* cmd, volatile uint32_t* flags) {
    check_null(cmd);

    if (cmd->cla != CLA) {
        THROW(EXC_CLASS);
    }

    int result = 0;
    switch (cmd->ins) {
        case INS_VERSION:
            result = handle_apdu_version();
            break;
        case INS_GIT:
            result = handle_apdu_git();
            break;
        case INS_GET_PUBLIC_KEY:
        case INS_PROMPT_PUBLIC_KEY:
        case INS_AUTHORIZE_BAKING:
            result = handle_apdu_get_public_key(cmd, flags);
            break;
        case INS_DEAUTHORIZE:
            result = handle_apdu_deauthorize(cmd);
            break;
        case INS_SETUP:
            result = handle_apdu_setup(cmd, flags);
            break;
        case INS_RESET:
            result = handle_apdu_reset(cmd, flags);
            break;
        case INS_QUERY_AUTH_KEY:
            result = handle_apdu_query_auth_key();
            break;
        case INS_QUERY_AUTH_KEY_WITH_CURVE:
            result = handle_apdu_query_auth_key_with_curve();
            break;
        case INS_QUERY_MAIN_HWM:
            result = handle_apdu_main_hwm();
            break;
        case INS_QUERY_ALL_HWM:
            result = handle_apdu_all_hwm();
            break;
        case INS_SIGN:
        case INS_SIGN_WITH_HASH:
            result = handle_apdu_sign(cmd, flags);
            break;
        case INS_HMAC:
            result = handle_apdu_hmac(cmd);
            break;
        default:
            THROW(EXC_INVALID_INS);
    }
    return result;
}
