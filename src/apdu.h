/* Tezos Ledger application - Common apdu primitives

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
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

#pragma once

#include "buffer.h"
#include "exception.h"
#include "keys.h"
#include "parser.h"
#include "types.h"
#include "io.h"
#include "ui.h"

#include "os.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Codes of handled instructions
 *
 */
#define INS_VERSION                   0x00u
#define INS_AUTHORIZE_BAKING          0x01u
#define INS_GET_PUBLIC_KEY            0x02u
#define INS_PROMPT_PUBLIC_KEY         0x03u
#define INS_SIGN                      0x04u
#define INS_SIGN_UNSAFE               0x05u
#define INS_RESET                     0x06u
#define INS_QUERY_AUTH_KEY            0x07u
#define INS_QUERY_MAIN_HWM            0x08u
#define INS_GIT                       0x09u
#define INS_SETUP                     0x0Au
#define INS_QUERY_ALL_HWM             0x0Bu
#define INS_DEAUTHORIZE               0x0Cu
#define INS_QUERY_AUTH_KEY_WITH_CURVE 0x0Du
#define INS_HMAC                      0x0Eu
#define INS_SIGN_WITH_HASH            0x0Fu

/**
 * @brief Dispatch APDU command received to the right handler
 *
 * @param cmd: structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int apdu_dispatcher(const command_t* cmd);

/**
 * @brief Sends a reject exception
 *
 * @return true
 */
static inline bool reject(void) {
    io_send_sw(EXC_REJECT);
    return true;
}

/**
 * @brief Provides the public key in the apdu response
 *
 *        Expects validated pin
 *
 * @param pubkey: public key
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int provide_pubkey(cx_ecfp_public_key_t const* const pubkey);
