/* Tezos Ledger application - Global strcuture handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
   Copyright 2021 Ledger
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

#include "types.h"

#include "bolos_target.h"

#include "operations.h"
#include "ui.h"
#include "ui_screensaver.h"

/**
 * @brief Zeros out all globals that can keep track of APDU instruction state
 *
 *        Notably this does *not* include UI state
 *
 */
void clear_apdu_globals(void);

/**
 * @brief Zeros out all application-specific globals and SDK-specific UI/exchange buffers
 *
 */
void init_globals(void);

/**
 * @brief Toggle high watermark tracking by Ledger.
 *
 * if its off, the responsibility to track watermark for blocks/attestation signed falls on the
 * signer being used.
 */
void toggle_hwm(void);

/// Maximum number of bytes in a single APDU
#define MAX_APDU_SIZE 235u

/// Our buffer must accommodate any remainder from hashing and the next message at once.
#define TEZOS_BUFSIZE (BLAKE2B_BLOCKBYTES + MAX_APDU_SIZE)

#define MAX_SIGNATURE_SIZE 100u

/**
 * @brief This structure represents the state needed to handle HMAC
 *
 */
typedef struct {
    uint8_t signed_hmac_key[MAX_SIGNATURE_SIZE];     ///< buffer to hold the signed hmac key
    uint8_t hashed_signed_hmac_key[CX_SHA512_SIZE];  ///< buffer to hold the hashed signed hmac key
    uint8_t hmac[CX_SHA256_SIZE];                    ///< buffer to hold the hmac result
} apdu_hmac_state_t;

/**
 * @brief This structure represents the state needed to hash messages
 *
 */
typedef struct {
    cx_blake2b_t state;  ///< blake2b state
    bool initialized;    ///< if the state has already been initialized
} blake2b_hash_state_t;

/**
 * @brief This structure represents the state needed to sign messages
 *
 */
typedef struct {
    /// 0-index is the initial setup packet, 1 is first packet to hash, etc.
    uint8_t packet_index;

    /// state to hold the current parsed bakind data
    parsed_baking_data_t parsed_baking_data;

    /// operation read, used for checks
    struct {
        bool is_valid;                    ///< if the parsed operation group is considered as valid
        struct parsed_operation_group v;  ///< current parsed operation group
    } maybe_ops;

    /// buffer to hold the current message part and the  previous message hash
    uint8_t message_data[TEZOS_BUFSIZE];
    size_t message_data_length;  ///< length of message data

    blake2b_hash_state_t hash_state;     ///< current blake2b hash state
    uint8_t final_hash[SIGN_HASH_SIZE];  ///< buffer to hold hash of all the message
#ifndef TARGET_NANOS
    uint8_t message[MAX_APDU_SIZE];  ///< buffer to hold last packet message
    size_t message_len;              ///< size of the message last packet
#endif

    magic_byte_t magic_byte;         ///< current magic byte read
    struct parse_state parse_state;  ///< current parser state
} apdu_sign_state_t;

/**
 * @brief This structure holds all structure needed
 *
 */
typedef struct {
    /// dynamic display state
    struct {
        /// Callback function if user accepted prompt.
        ui_callback_t ok_callback;
        /// Callback function if user rejected prompt.
        ui_callback_t cxl_callback;
#ifdef HAVE_BAGL
        /// If the low-cost display mode is enabled
        bool low_cost_display_mode;
        /// Screensaver context
        ux_screensaver_state_t screensaver_state;
#endif  // TARGET_NANOS
    } dynamic_display;

    bip32_path_with_curve_t path_with_curve;  ///< holds the bip32 path and curve of the current key

    /// apdu handling state
    struct {
        union {
            apdu_sign_state_t sign;  ///< state used to handle signing

            /// state used to handle reset
            struct {
                level_t reset_level;  ///< requested reset level
            } baking;

            /// state used to handle setup
            struct {
                chain_id_t main_chain_id;  ///< requested new main chain id
                /// requested new HWM information
                struct {
                    level_t main;  ///< level requested to be set on main HWM
                    level_t test;  ///< level requested to be set on test HWM
                } hwm;
            } setup;

            apdu_hmac_state_t hmac;  ///< state used to handle hmac
        } u;
    } apdu;

    baking_data hwm_data;  ///< baking HWM data in RAM
} globals_t;

extern globals_t global;

#define g_hwm global.hwm_data

extern baking_data const N_data_real;
#define N_data (*(volatile baking_data *) PIC(&N_data_real))

/**
 * @brief Selects a HWM for a given chain id depending on the ram
 *
 *        Selects the main HWM of the ram if the main chain of the ram
 *        is not defined, or if the given chain matches the main chain
 *        of the ram. Selects the test HWM of the ram otherwise.
 *
 * @param chain_id: chain id
 * @return high_watermark_t*: selected HWM
 */
high_watermark_t *select_hwm_by_chain(chain_id_t const chain_id);

/**
 * @brief Updates a single variable in NVRAM baking_data.
 *
 * @param variable: defines the name of the variable to be updated in NVRAM
 */
#define UPDATE_NVRAM_VAR(variable)                   \
    if (!N_data_real.hwm_disabled) {                 \
        nvm_write((void *) &(N_data.variable),       \
                  &global.hwm_data.variable,         \
                  sizeof(global.hwm_data.variable)); \
    }

/**
 * @brief Properly updates an entire NVRAM struct to prevent any clobbering of data
 *
 */
#define UPDATE_NVRAM nvm_write((void *) &(N_data), &global.hwm_data, sizeof(global.hwm_data));
