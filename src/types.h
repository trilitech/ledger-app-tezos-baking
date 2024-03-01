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

/**
 * NOTE: There are *two* ways that "key type" or "curve code" are represented in
 * this code base:
 *   1. `derivation_type` represents how a key will be derived from the seed. It
 *      is almost the same as `signature_type` but allows for multiple derivation
 *      strategies for ed25519. This type is often parsed from the APDU
 *      instruction. See `parse_derivation_type` for the mapping.
 *   2. `signature_type` represents how a key will be used for signing.
 *      The mapping from `derivation_type` to `signature_type` is injective.
 *      See `derivation_type_to_signature_type`.
 *      This type is parsed from Tezos data headers. See the relevant parsing
 *      code for the mapping.
 */
typedef enum {
    DERIVATION_TYPE_SECP256K1 = 1,
    DERIVATION_TYPE_SECP256R1 = 2,
    DERIVATION_TYPE_ED25519 = 3,
    DERIVATION_TYPE_BIP32_ED25519 = 4
} derivation_type_t;
typedef enum {
    SIGNATURE_TYPE_UNSET = 0,
    SIGNATURE_TYPE_SECP256K1 = 1,
    SIGNATURE_TYPE_SECP256R1 = 2,
    SIGNATURE_TYPE_ED25519 = 3
} signature_type_t;

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
 * @brief Generic APDU instruction handler
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
typedef size_t (*apdu_handler)(uint8_t instruction, volatile uint32_t *flags);

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
 * @brief String generator callback
 *
 *        Uses K&R style declaration to avoid being stuck on const
 *        void *, to avoid having to cast the function pointers
 *
 * @param buffer: output buffer
 * @param buffer_size: output size
 * @param data: data
 */
typedef void (*string_generation_callback)(
    /* char *buffer, size_t buffer_size, const void *data */);

/**
 * @brief Pair of public and private key
 *
 */
typedef struct {
    cx_ecfp_public_key_t public_key;    ///< public key
    cx_ecfp_private_key_t private_key;  ///< private key
} key_pair_t;

#define MAX_BIP32_LEN 10

/**
 * @brief This structure represents a bip32 path
 *
 */
typedef struct {
    uint8_t length;                      ///< length of the path
    uint32_t components[MAX_BIP32_LEN];  ///< array of components
} bip32_path_t;

/**
 * @brief Copies a bip32 path
 *
 * @param out: bip32 path output
 * @param in: bip32 path copied
 */
static inline void copy_bip32_path(bip32_path_t *const out, bip32_path_t volatile const *const in) {
    check_null(out);
    check_null(in);
    memcpy(out->components, in->components, in->length * sizeof(*in->components));
    out->length = in->length;
}

/**
 * @brief Checks that two bip32 paths are equals
 *
 * @param a: first bip32 path
 * @param b: second bip32 path
 * @return bool: result
 */
static inline bool bip32_paths_eq(bip32_path_t volatile const *const a,
                                  bip32_path_t volatile const *const b) {
    return a == b || (a != NULL && b != NULL && a->length == b->length &&
                      memcmp((void const *) a->components,
                             (void const *) b->components,
                             a->length * sizeof(*a->components)) == 0);
}

/**
 * @brief This structure represents a bip32 path and its curve
 *
 *        Defines a key when associates with a mnemonic seed
 *
 */
typedef struct {
    bip32_path_t bip32_path;            ///< bip32 path of the key
    derivation_type_t derivation_type;  ///< curve of the key
} bip32_path_with_curve_t;

/**
 * @brief Copies a bip32 path and its curve
 *
 * @param out: output
 * @param in: bip32 path and curve copied
 */
static inline void copy_bip32_path_with_curve(bip32_path_with_curve_t *const out,
                                              bip32_path_with_curve_t volatile const *const in) {
    check_null(out);
    check_null(in);
    copy_bip32_path(&out->bip32_path, &in->bip32_path);
    out->derivation_type = in->derivation_type;
}

/**
 * @brief Checks that two bip32 paths with their paths are equals
 *
 * @param a: first bip32 path and curve
 * @param b: first bip32 path and curve
 * @return bool: result
 */
static inline bool bip32_path_with_curve_eq(bip32_path_with_curve_t volatile const *const a,
                                            bip32_path_with_curve_t volatile const *const b) {
    return a == b || (a != NULL && b != NULL && bip32_paths_eq(&a->bip32_path, &b->bip32_path) &&
                      a->derivation_type == b->derivation_type);
}

/**
 * @brief This structure represents a High Watermark (HWM)
 *
 */
typedef struct {
    level_t highest_level;        ///< highest level seen
    round_t highest_round;        ///< highest round seen
    bool had_attestation;         ///< if an attestation has been seen at current level/round
    bool had_preattestation;      ///< if a pre-attestation has been seen at current level/round
    bool migrated_to_tenderbake;  ///< if chain has migrated to tenderbake
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
} nvram_data;

#define SIGN_HASH_SIZE 32u  // TODO: Rename or use a different constant.

#define PKH_STRING_SIZE 40u  // includes null byte // TODO: use sizeof for this.
#define PROTOCOL_HASH_BASE58_STRING_SIZE \
    sizeof("ProtoBetaBetaBetaBetaBetaBetaBetaBetaBet11111a5ug96")

#define MAX_SCREEN_STACK_SIZE 7u  // Maximum number of screens in a flow.
#define PROMPT_WIDTH          16u
#define VALUE_WIDTH           PROTOCOL_HASH_BASE58_STRING_SIZE

// TODO: Rename to KEY_HASH_SIZE
#define HASH_SIZE 20u

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
    uint8_t originated;  ///< a lightweight bool
    signature_type_t
        signature_type;       ///< 0 in originated case
                              ///< An implicit contract with signature_type of 0 means not present
    uint8_t hash[HASH_SIZE];  ///< hash of the contract
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

#define INS_MAX 0x0Fu

#define APDU_INS(x)                                                        \
    ({                                                                     \
        _Static_assert(x <= INS_MAX, "APDU instruction is out of bounds"); \
        x;                                                                 \
    })

#define CUSTOM_MAX(a, b)                   \
    ({                                     \
        __typeof__(a) ____a_ = (a);        \
        __typeof__(b) ____b_ = (b);        \
        ____a_ > ____b_ ? ____a_ : ____b_; \
    })

#define CUSTOM_MIN(a, b)                   \
    ({                                     \
        __typeof__(a) ____a_ = (a);        \
        __typeof__(b) ____b_ = (b);        \
        ____a_ < ____b_ ? ____a_ : ____b_; \
    })
