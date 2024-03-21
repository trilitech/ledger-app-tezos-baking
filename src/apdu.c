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

int provide_pubkey(bip32_path_with_curve_t const* const path_with_curve) {
    check_null(path_with_curve);

    // 100 = MAX(SIGNATURE_LEN)
    uint8_t resp[1u + 100u] = {0};
    size_t offset = 0;

    // Application could be PIN-locked, and pubkey->W_len would then be 0,
    // so throwing an error rather than returning an empty key
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        THROW(EXC_SECURITY);
    }

    cx_ecfp_public_key_t pubkey = {0};
    CX_THROW(generate_public_key(&pubkey, path_with_curve));

    resp[offset] = pubkey.W_len;
    offset++;
    memmove(resp + offset, pubkey.W, pubkey.W_len);
    offset += pubkey.W_len;

    return io_send_response_pointer(resp, offset, SW_OK);
}

/**
 * @brief Gets the version
 *
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
static int handle_version(void) {
    return io_send_response_pointer((const uint8_t*) &version, sizeof(version_t), SW_OK);
}

/**
 * @brief Gets the git commit
 *
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
static int handle_git(void) {
    return io_send_response_pointer((const uint8_t*) &COMMIT, sizeof(COMMIT), SW_OK);
}

#define CLA 0x80  /// The only APDU class that will be used

/// Packet indexes
#define P1_FIRST       0x00u  /// First packet
#define P1_NEXT        0x01u  /// Other packet
#define P1_LAST_MARKER 0x80u  /// Last packet

int apdu_dispatcher(const command_t* cmd) {
    check_null(cmd);

    if (cmd->lc > MAX_APDU_SIZE) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    if (cmd->cla != CLA) {
        THROW(EXC_CLASS);
    }

    int result = 0;
    buffer_t buf = {0};
    derivation_type_t derivation_type = DERIVATION_TYPE_UNSET;

#define ASSERT_NO_P1                \
    do {                            \
        if (cmd->p1 != 0u) {        \
            THROW(EXC_WRONG_PARAM); \
        }                           \
    } while (0)

#define ASSERT_NO_P2                \
    do {                            \
        if (cmd->p2 != 0u) {        \
            THROW(EXC_WRONG_PARAM); \
        }                           \
    } while (0)

#define READ_P2_DERIVATION_TYPE                           \
    do {                                                  \
        derivation_type = parse_derivation_type(cmd->p2); \
        if (derivation_type == DERIVATION_TYPE_UNSET) {   \
            THROW(EXC_WRONG_PARAM);                       \
        }                                                 \
    } while (0)

#define ASSERT_NO_DATA               \
    do {                             \
        if (cmd->data != NULL) {     \
            THROW(EXC_WRONG_VALUES); \
        }                            \
    } while (0)

#define READ_DATA            \
    do {                     \
        buf.ptr = cmd->data; \
        buf.size = cmd->lc;  \
        buf.offset = 0u;     \
    } while (0)

    switch (cmd->ins) {
        case INS_VERSION:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_version();

            break;
        case INS_GIT:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_git();

            break;
        case INS_GET_PUBLIC_KEY:
        case INS_PROMPT_PUBLIC_KEY:
        case INS_AUTHORIZE_BAKING:

            ASSERT_NO_P1;
            READ_P2_DERIVATION_TYPE;
            READ_DATA;

            bool authorize = cmd->ins == INS_AUTHORIZE_BAKING;
            bool prompt = (cmd->ins == INS_AUTHORIZE_BAKING) || (cmd->ins == INS_PROMPT_PUBLIC_KEY);

            result = handle_get_public_key(&buf, derivation_type, authorize, prompt);

            break;
        case INS_DEAUTHORIZE:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_deauthorize();

            break;
        case INS_SETUP:

            ASSERT_NO_P1;
            READ_P2_DERIVATION_TYPE;
            READ_DATA;

            result = handle_setup(&buf, derivation_type);

            break;
        case INS_RESET:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            READ_DATA;

            result = handle_reset(&buf);

            break;
        case INS_QUERY_AUTH_KEY:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_query_auth_key();

            break;
        case INS_QUERY_AUTH_KEY_WITH_CURVE:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_query_auth_key_with_curve();

            break;
        case INS_QUERY_MAIN_HWM:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_query_main_hwm();

            break;
        case INS_QUERY_ALL_HWM:

            ASSERT_NO_P1;
            ASSERT_NO_P2;
            ASSERT_NO_DATA;

            result = handle_query_all_hwm();

            break;
        case INS_SIGN:
        case INS_SIGN_WITH_HASH:
            if (os_global_pin_is_validated() != BOLOS_UX_OK) {
                THROW(EXC_SECURITY);
            }

            switch (cmd->p1 & ~P1_LAST_MARKER) {
                case P1_FIRST:

                    READ_P2_DERIVATION_TYPE;
                    READ_DATA;

                    result = select_signing_key(&buf, derivation_type);

                    break;
                case P1_NEXT:

                    READ_DATA;

                    bool with_hash = cmd->ins == INS_SIGN_WITH_HASH;
                    bool last = (cmd->p1 & P1_LAST_MARKER) != 0;

                    result = handle_sign(&buf, last, with_hash);

                    break;
                default:
                    THROW(EXC_WRONG_PARAM);
            }

            break;
        case INS_HMAC:

            ASSERT_NO_P1;
            READ_P2_DERIVATION_TYPE;
            READ_DATA;

            result = handle_hmac(&buf, derivation_type);

            break;
        default:
            THROW(EXC_INVALID_INS);
    }
    return result;
}
