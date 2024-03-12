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
#include "protocol.h"

#include "cx.h"
#include "types.h"

// Wire format that gets parsed into `signature_type`.
typedef struct {
    uint8_t v;
} __attribute__((packed)) raw_tezos_header_signature_type_t;

struct operation_group_header {
    uint8_t magic_byte;
    uint8_t hash[32];
} __attribute__((packed));

struct implicit_contract {
    raw_tezos_header_signature_type_t signature_type;
    uint8_t pkh[HASH_SIZE];
} __attribute__((packed));

struct delegation_contents {
    raw_tezos_header_signature_type_t signature_type;
    uint8_t hash[HASH_SIZE];
} __attribute__((packed));

struct int_subparser_state {
    uint32_t lineno;  // Has to be in _all_ members of the subparser union.
    uint64_t value;   // Still need to fix this.
    uint8_t shift;
};

struct nexttype_subparser_state {
    uint32_t lineno;
    union {
        raw_tezos_header_signature_type_t sigtype;

        struct operation_group_header ogh;

        struct implicit_contract ic;

        struct delegation_contents dc;

        uint8_t raw[1];
    } body;
    uint32_t fill_idx;
};

union subparser_state {
    struct int_subparser_state integer;
    struct nexttype_subparser_state nexttype;
};

struct parse_state {
    int16_t op_step;
    union subparser_state subparser_state;
    enum operation_tag tag;
};

// Allows arbitrarily many "REVEAL" operations but only one operation of any other type,
// which is the one it puts into the group.
bool parse_operations(struct parsed_operation_group *const out,
                      uint8_t const *const data,
                      size_t length,
                      derivation_type_t curve,
                      bip32_path_t const *const bip32_path);

bool parse_operations_final(struct parse_state *const state,
                            struct parsed_operation_group *const out);
