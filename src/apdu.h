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

#include "exception.h"
#include "keys.h"
#include "types.h"
#include "ui.h"

#include "os.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Offset for the different items in an APDU
 *
 */
#define OFFSET_CLA   0  /// APDU class
#define OFFSET_INS   1  /// instruction code
#define OFFSET_P1    2  /// packet index
#define OFFSET_CURVE 3  /// key curve: derivation_type_t
#define OFFSET_LC    4  /// length of payload
#define OFFSET_CDATA 5  /// payload

/**
 * @brief Codes of handled instructions
 *
 */
#define INS_VERSION                   0x00
#define INS_AUTHORIZE_BAKING          0x01
#define INS_GET_PUBLIC_KEY            0x02
#define INS_PROMPT_PUBLIC_KEY         0x03
#define INS_SIGN                      0x04
#define INS_SIGN_UNSAFE               0x05
#define INS_RESET                     0x06
#define INS_QUERY_AUTH_KEY            0x07
#define INS_QUERY_MAIN_HWM            0x08
#define INS_GIT                       0x09
#define INS_SETUP                     0x0A
#define INS_QUERY_ALL_HWM             0x0B
#define INS_DEAUTHORIZE               0x0C
#define INS_QUERY_AUTH_KEY_WITH_CURVE 0x0D
#define INS_HMAC                      0x0E
#define INS_SIGN_WITH_HASH            0x0F

/**
 * @brief Loops indefinitely while handling the incoming apdus
 *
 * @param handlers: list of apdu handler
 * @param handlers_size: updated offset of the apdu response
 */
void main_loop(apdu_handler const* const handlers, size_t const handlers_size)
    __attribute__((noreturn));

/**
 * @brief Tags as successful apdu response
 *
 * @param tx: current offset of the apdu response
 * @return size_t: updated offset of the apdu response
 */
static inline size_t finalize_successful_send(size_t tx) {
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    return tx;
}

/**
 * @brief Sends the apdu response asynchronously
 *
 * @param tx: current offset of the apdu response
 */
static inline void delayed_send(size_t tx) {
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
}

/**
 * @brief Sends asynchronously a reject exception
 *
 * @return true
 */
static inline bool delay_reject(void) {
    size_t tx = 0;
    G_io_apdu_buffer[tx++] = EXC_REJECT >> 8;
    G_io_apdu_buffer[tx++] = EXC_REJECT & 0xFF;
    delayed_send(tx);
    return true;
}

/**
 * @brief Provides the public key in the apdu response
 *
 *        Expects validated pin
 *
 * @param io_buffer: apdu response buffer
 * @param pubkey: public key
 * @return size_t: offset of the apdu response
 */
size_t provide_pubkey(uint8_t* const io_buffer, cx_ecfp_public_key_t const* const pubkey);

/**
 * @brief Handles unknown instructions
 *
 *        Raises an invalid instruction exception
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_error(uint8_t instruction, volatile uint32_t* flags);

/**
 * @brief Handles VERSION instruction
 *
 *        Fills apdu response with the app version
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_version(uint8_t instruction, volatile uint32_t* flags);

/**
 * @brief Handles GIT instruction
 *
 *        Fills apdu response with the app commit
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_git(uint8_t instruction, volatile uint32_t* flags);
