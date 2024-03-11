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

/**
 * @brief Zeros out all globals that can keep track of APDU instruction state
 *
 *        Notably this does *not* include UI state
 *
 */
void clear_apdu_globals(void);

/**
 * @brief string_generation_callback for chains
 *
 * @param out: output buffer
 * @param out_size: output size
 * @param data: chain_id
 */
void copy_chain(char *out, size_t out_size, void *data);

/**
 * @brief string_generation_callback for keys
 *
 * @param out: output buffer
 * @param out_size: output size
 * @param data: bip32 path curve key
 */
void copy_key(char *out, size_t out_size, void *data);

/**
 * @brief string_generation_callback for high watermarks
 *
 * @param out: output buffer
 * @param out_size: output size
 * @param data: high watermark
 */
void copy_hwm(char *out, size_t out_size, void *data);

/**
 * @brief Zeros out all application-specific globals and SDK-specific UI/exchange buffers
 *
 */
void init_globals(void);

/// Maximum number of bytes in a single APDU
#define MAX_APDU_SIZE 235

/// Our buffer must accommodate any remainder from hashing and the next message at once.
#define TEZOS_BUFSIZE (BLAKE2B_BLOCKBYTES + MAX_APDU_SIZE)

#define PRIVATE_KEY_DATA_SIZE 64
#define MAX_SIGNATURE_SIZE    100

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

    uint8_t magic_byte;              ///< current magic byte read
    struct parse_state parse_state;  ///< current parser state
} apdu_sign_state_t;

/**
 * @brief This structure represents data used to compute what we need to display on the screen.
 *
 */
struct screen_data {
    char *title;                             ///< title of the screen
    string_generation_callback callback_fn;  ///< callback to convert the data to string
    void *data;                              ///< value to display on the screen
};

/**
 * @brief State of the dynamic display
 *
 *        Used to keep track on whether we are displaying screens inside the stack, or outside the
 *        stack (for example confirmation screens)
 *
 */
enum e_state {
    STATIC_SCREEN,
    DYNAMIC_SCREEN,
};

/**
 * @brief This structure holds all structure needed
 *
 */
typedef struct {
    /// dynamic display state
    struct {
        struct screen_data screen_stack[MAX_SCREEN_STACK_SIZE];
        enum e_state current_state;  ///< State of the dynamic display

        /// Size of the screen stack
        uint8_t screen_stack_size;

        /// Current index in the screen_stack.
        uint8_t formatter_index;

        /// Callback function if user accepted prompt.
        ui_callback_t ok_callback;
        /// Callback function if user rejected prompt.
        ui_callback_t cxl_callback;

        /// Title to be displayed on the screen.
        char screen_title[PROMPT_WIDTH + 1];
        /// Value to be displayed on the screen.
        char screen_value[VALUE_WIDTH + 1];
        /// Screensaver is on/off.
        bool is_blank_screen;
    } dynamic_display;

    apdu_handler handlers[INS_MAX + 1];       ///< all handlers
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

        /// state used to store baking authorizing data
        struct {
            nvram_data new_data;  ///< Staging area for setting N_data
        } baking_auth;
    } apdu;
} globals_t;

extern globals_t global;

extern nvram_data const N_data_real;
#define N_data (*(volatile nvram_data *) PIC(&N_data_real))

/**
 * @brief Selects a HWM for a given chain id depending on the ram
 *
 *        Selects the main HWM of the ram if the main chain of the ram
 *        is not defined, or if the given chain matches the main chain
 *        of the ram. Selects the test HWM of the ram otherwise.
 *
 * @param chain_id: chain id
 * @param ram: ram
 * @return high_watermark_t*: selected HWM
 */
high_watermark_t volatile *select_hwm_by_chain(chain_id_t const chain_id,
                                               nvram_data volatile *const ram);

/**
 * @brief Properly updates NVRAM data to prevent any clobbering of data
 *
 * @param out_name: defines the name of a pointer to the nvram_data struct
 * @param body: defines the code to apply updates
 */
#define UPDATE_NVRAM(out_name, body)                                                    \
    ({                                                                                  \
        nvram_data *const out_name = &global.apdu.baking_auth.new_data;                 \
        memcpy(&global.apdu.baking_auth.new_data,                                       \
               (nvram_data const *const) &N_data,                                       \
               sizeof(global.apdu.baking_auth.new_data));                               \
        body;                                                                           \
        nvm_write((void *) &N_data, &global.apdu.baking_auth.new_data, sizeof(N_data)); \
    })
