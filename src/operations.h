/* Tezos Ledger application - Operation parsing

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
   Copyright 2020 Obsidian Systems

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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "keys.h"

#include "cx.h"
#include "types.h"

/**
 * @brief Wire format that gets parsed into `signature_type`
 *
 */
typedef struct {
    uint8_t v;  ///< value of the type of header signature
} __attribute__((packed)) raw_tezos_header_signature_type_t;

/**
 * @brief Wire representation of operation group header
 *
 */
struct operation_group_header {
    uint8_t hash[32];  ///< hash of the operation
} __attribute__((packed));

/**
 * @brief Wire representation of implicit contract
 *
 */
struct implicit_contract {
    raw_tezos_header_signature_type_t signature_type;  ///< type of the contract signature
    uint8_t pkh[HASH_SIZE];                            ///< raw public key hash
} __attribute__((packed));

/**
 * @brief Wire representation of delegation
 *
 */
struct delegation_contents {
    raw_tezos_header_signature_type_t signature_type;  ///< type of the delegate signature
    uint8_t hash[HASH_SIZE];                           ///< raw delegate
} __attribute__((packed));

/**
 * @brief This structure represents the state of a Z parser
 *
 */
struct int_subparser_state {
    uint32_t lineno;  ///< line number
                      ///< Has to be in _all_ members of the subparser union.
    uint64_t value;   ///< Read value
                      /// Still need to fix this.
    uint8_t shift;    ///< Z shift
};

/**
 * @brief This structure represents the state of a wire type parser
 *
 *        Allows to read data using wire representation types
 *
 *        Fills body using raw and an increasing fill_idx
 *
 */
struct nexttype_subparser_state {
    uint32_t lineno;  ///< line number

    /// union of all wire structure
    union {
        raw_tezos_header_signature_type_t sigtype;  ///< wire signature_type

        struct operation_group_header ogh;  ///< wire operation group header

        struct implicit_contract ic;  ///< wire implicit contract

        struct delegation_contents dc;  ///< wire delegation content

        uint8_t raw[1];  ///< raw array to fill the body
    } body;
    uint32_t fill_idx;  ///< current fill index
};

/**
 * @brief This structure represents the union of all subparsers
 *
 */
union subparser_state {
    struct int_subparser_state integer;        ///< state of a n integer parser
    struct nexttype_subparser_state nexttype;  ///< state of a wire type parser
};

/**
 * @brief This structure represents the parsing state
 *
 */
struct parse_state {
    int16_t op_step;                        ///< current parsing step
    union subparser_state subparser_state;  ///< state of subparser
    enum operation_tag tag;                 ///< current operation tag
};

/**
 * @brief Parses a group of operation
 *
 *        Allows arbitrarily many "REVEAL" operations but only one
 *        operation of any other type, which is the one it puts into
 *        the group.
 *
 *        Some checks are carried out during the parsing using a key using a key
 *
 * @param buf: input operation
 * @param out: parsing output
 * @param curve: curve of the key
 * @param bip32_path: bip32 path of the key
 * @return bool: returns true on success
 */
bool parse_operations(buffer_t *buf,
                      struct parsed_operation_group *const out,
                      derivation_type_t curve,
                      bip32_path_t const *const bip32_path);

/**
 * @brief Checks parsing has been completed successfully
 *
 * @param state: parsing state
 * @param out: parsing output
 * @return bool: returns true on success
 */
bool parse_operations_final(struct parse_state *const state,
                            struct parsed_operation_group *const out);
