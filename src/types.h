/* Tezos Ledger application - Types

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

#pragma once

#include "exception.h"
#include "os.h"
#include "os_io_seproxyhal.h"

#include <stdbool.h>
#include <string.h>

#include "keys.h"

/**
 * @brief Type of baking message
 *
 */
typedef enum {
    BAKING_TYPE_BLOCK,
    BAKING_TYPE_ATTESTATION,
    BAKING_TYPE_PREATTESTATION
} baking_type_t;

/**
 * @brief magic byte of operations
 * See: https://tezos.gitlab.io/user/key-management.html#signer-requests
 */
typedef enum {
    MAGIC_BYTE_UNSAFE_OP = 0x03u,       /// magic byte of an operation
    MAGIC_BYTE_BLOCK = 0x11u,           /// magic byte of a block
    MAGIC_BYTE_PREATTESTATION = 0x12u,  /// magic byte of a pre-attestation
    MAGIC_BYTE_ATTESTATION = 0x13u,     /// magic byte of an attestation
} magic_byte_t;

typedef uint32_t level_t;
typedef uint32_t round_t;

#define CHAIN_ID_BASE58_STRING_SIZE sizeof("NetXdQprcVkpaWU")

#define MAX_INT_DIGITS 20u

/**
 * @brief This structure represents chain id
 *
 */
typedef struct {
    uint32_t v;  ///< value of the chain id
} chain_id_t;

// Mainnet Chain ID: NetXdQprcVkpaWU
static chain_id_t const mainnet_chain_id = {.v = 0x7A06A770};

/**
 * @brief UI callback
 *
 * @return bool: true to go back to idle screen
 */
typedef bool (*ui_callback_t)(void);

/**
 * @brief This structure represents a High Watermark (HWM)
 *
 */
typedef struct {
    level_t highest_level;    ///< highest level seen
    round_t highest_round;    ///< highest round seen
    bool had_attestation;     ///< if an attestation has been seen at current level/round
    bool had_preattestation;  ///< if a pre-attestation has been seen at current level/round
} high_watermark_t;

/**
 * @brief This structure represents data store in NVRAM
 *
 */
typedef struct {
    chain_id_t main_chain_id;  ///< main chain id

    /// high watermarks information
    struct {
        high_watermark_t main;  ///< HWM of main
        high_watermark_t test;  ///< HWM of test
    } hwm;

    bip32_path_with_curve_t baking_key;  ///< authorized key
    bool hwm_disabled;                   /**< Set HWM setting on/off,
                                              e.g. if you are using signer assisted HWM,
                                              no need to track HWM using Ledger.*/
} baking_data;

#define SIGN_HASH_SIZE 32u

#define PKH_STRING_SIZE 40u  // includes null byte
#define HWM_STATUS_SIZE 9u   // HWM status takes values Enabled and Disabled.
#define PROTOCOL_HASH_BASE58_STRING_SIZE \
    sizeof("ProtoBetaBetaBetaBetaBetaBetaBetaBetaBet11111a5ug96")

/**
 * @brief This structure represents the content of a parsed baking data
 *
 */
typedef struct {
    chain_id_t chain_id;  ///< chain id
    baking_type_t type;   ///< kind of the baking message
    level_t level;        ///< level of the  baking message
    round_t round;        ///< round of the  baking message
    bool is_tenderbake;   ///< if belongs to the tenderbake consensus protocol
} parsed_baking_data_t;

/**
 * @brief This structure represents information about parsed contract
 *
 */
typedef struct parsed_contract {
    uint8_t originated;              ///< a lightweight bool
    signature_type_t signature_type; /**< 0 in originated case
                                     An implicit contract with signature_type of 0
                                     means not present*/
    uint8_t hash[KEY_HASH_SIZE];     ///< hash of the contract
} parsed_contract_t;

/**
 * @brief Tag of operation
 *
 */
enum operation_tag {
    OPERATION_TAG_NONE = -1,  // Sentinal value, as 0 is possibly used for something
    OPERATION_TAG_REVEAL = 107,
    OPERATION_TAG_DELEGATION = 110,
};

/**
 * @brief This structure represents information about parsed operation
 *
 */
struct parsed_operation {
    enum operation_tag tag;              ///< operation tag
    struct parsed_contract source;       ///< source of the operation
    struct parsed_contract destination;  ///< destination of the operation
};

/**
 * @brief This structure represents information about parsed a bundle of operations
 *
 *        Except for reveals, only one operation can be parsed per bundle
 *
 */
struct parsed_operation_group {
    cx_ecfp_public_key_t public_key;    ///< signer public key
    uint64_t total_fee;                 ///< sum of all fees
    uint64_t total_storage_limit;       ///< sum of all storage limits
    bool has_reveal;                    ///< if the bundle contains at least a reveal
    struct parsed_contract signing;     ///< contract form of signer
    struct parsed_operation operation;  ///< operation parsed
};

#define CUSTOM_MAX(a, b)                     \
    ({                                       \
        __typeof__(a) ____a_ = (a);          \
        __typeof__(b) ____b_ = (b);          \
        (____a_ > ____b_) ? ____a_ : ____b_; \
    })

#define CUSTOM_MIN(a, b)                     \
    ({                                       \
        __typeof__(a) ____a_ = (a);          \
        __typeof__(b) ____b_ = (b);          \
        (____a_ < ____b_) ? ____a_ : ____b_; \
    })
