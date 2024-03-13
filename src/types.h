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

// Type-safe versions of true/false
#undef true
#define true ((bool) 1)
#undef false
#define false ((bool) 0)

// NOTE: There are *two* ways that "key type" or "curve code" are represented in
// this code base:
//   1. `derivation_type` represents how a key will be derived from the seed. It
//      is almost the same as `signature_type` but allows for multiple derivation
//      strategies for ed25519. This type is often parsed from the APDU
//      instruction. See `parse_derivation_type` for the mapping.
//   2. `signature_type` represents how a key will be used for signing.
//      The mapping from `derivation_type` to `signature_type` is injective.
//      See `derivation_type_to_signature_type`.
//      This type is parsed from Tezos data headers. See the relevant parsing
//      code for the mapping.
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

typedef enum {
    BAKING_TYPE_BLOCK = 2,
    BAKING_TYPE_ATTESTATION = 3,
    BAKING_TYPE_PREATTESTATION = 4
} baking_type_t;

// Return number of bytes to transmit (tx)
typedef size_t (*apdu_handler)(uint8_t instruction, volatile uint32_t *flags);

typedef uint32_t level_t;
typedef uint32_t round_t;

#define CHAIN_ID_BASE58_STRING_SIZE sizeof("NetXdQprcVkpaWU")

#define MAX_INT_DIGITS 20

typedef struct {
    uint32_t v;
} chain_id_t;

// Mainnet Chain ID: NetXdQprcVkpaWU
static chain_id_t const mainnet_chain_id = {.v = 0x7A06A770};

// UI
typedef bool (*ui_callback_t)(void);  // return true to go back to idle screen

// Uses K&R style declaration to avoid being stuck on const void *, to avoid having to cast the
// function pointers.
typedef void (*string_generation_callback)(
    /* char *buffer, size_t buffer_size, const void *data */);

// Keys
typedef struct {
    cx_ecfp_public_key_t public_key;
    cx_ecfp_private_key_t private_key;
} key_pair_t;

// Baking Auth
#define MAX_BIP32_LEN 10

typedef struct {
    uint8_t length;
    uint32_t components[MAX_BIP32_LEN];
} bip32_path_t;

static inline void copy_bip32_path(bip32_path_t *const out, bip32_path_t volatile const *const in) {
    check_null(out);
    check_null(in);
    memcpy(out->components, (void *) in->components, in->length * sizeof(*in->components));
    out->length = in->length;
}

static inline bool bip32_paths_eq(bip32_path_t volatile const *const a,
                                  bip32_path_t volatile const *const b) {
    return a == b || (a != NULL && b != NULL && a->length == b->length &&
                      memcmp((void const *) a->components,
                             (void const *) b->components,
                             a->length * sizeof(*a->components)) == 0);
}

typedef struct {
    bip32_path_t bip32_path;
    derivation_type_t derivation_type;
} bip32_path_with_curve_t;

static inline void copy_bip32_path_with_curve(bip32_path_with_curve_t *const out,
                                              bip32_path_with_curve_t volatile const *const in) {
    check_null(out);
    check_null(in);
    copy_bip32_path(&out->bip32_path, &in->bip32_path);
    out->derivation_type = in->derivation_type;
}

static inline bool bip32_path_with_curve_eq(bip32_path_with_curve_t volatile const *const a,
                                            bip32_path_with_curve_t volatile const *const b) {
    return a == b || (a != NULL && b != NULL && bip32_paths_eq(&a->bip32_path, &b->bip32_path) &&
                      a->derivation_type == b->derivation_type);
}

typedef struct {
    level_t highest_level;
    round_t highest_round;
    bool had_attestation;
    bool had_preattestation;
    bool migrated_to_tenderbake;
} high_watermark_t;

typedef struct {
    chain_id_t main_chain_id;
    struct {
        high_watermark_t main;
        high_watermark_t test;
    } hwm;
    bip32_path_with_curve_t baking_key;
} nvram_data;

#define SIGN_HASH_SIZE 32  // TODO: Rename or use a different constant.

#define PKH_STRING_SIZE 40  // includes null byte // TODO: use sizeof for this.
#define PROTOCOL_HASH_BASE58_STRING_SIZE \
    sizeof("ProtoBetaBetaBetaBetaBetaBetaBetaBetaBet11111a5ug96")

#define MAX_SCREEN_STACK_SIZE 7  // Maximum number of screens in a flow.
#define PROMPT_WIDTH          16
#define VALUE_WIDTH           PROTOCOL_HASH_BASE58_STRING_SIZE

// Operations
#define PROTOCOL_HASH_SIZE 32

// TODO: Rename to KEY_HASH_SIZE
#define HASH_SIZE 20

typedef struct {
    chain_id_t chain_id;
    baking_type_t type;
    level_t level;
    round_t round;
    bool is_tenderbake;
} parsed_baking_data_t;

typedef struct parsed_contract {
    uint8_t originated;  // a lightweight bool
    signature_type_t
        signature_type;  // 0 in originated case
                         // An implicit contract with signature_type of 0 means not present
    uint8_t hash[HASH_SIZE];
} parsed_contract_t;

enum operation_tag {
    OPERATION_TAG_NONE = -1,  // Sentinal value, as 0 is possibly used for something
    OPERATION_TAG_REVEAL = 107,
    OPERATION_TAG_DELEGATION = 110,
};

struct parsed_operation {
    enum operation_tag tag;
    struct parsed_contract source;
    struct parsed_contract destination;

    uint32_t flags;  // Interpretation depends on operation type
};

struct parsed_operation_group {
    cx_ecfp_public_key_t public_key;  // compressed
    uint64_t total_fee;
    uint64_t total_storage_limit;
    bool has_reveal;
    struct parsed_contract signing;
    struct parsed_operation operation;
};

// Maximum number of APDU instructions
#define INS_MAX 0x0F

#define APDU_INS(x)                                                        \
    ({                                                                     \
        _Static_assert(x <= INS_MAX, "APDU instruction is out of bounds"); \
        x;                                                                 \
    })

#define STRCPY(buff, x)                                                         \
    ({                                                                          \
        _Static_assert(sizeof(buff) >= sizeof(x) && sizeof(*x) == sizeof(char), \
                       "String won't fit in buffer");                           \
        strcpy(buff, x);                                                        \
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
