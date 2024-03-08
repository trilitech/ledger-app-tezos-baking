/* Tezos Ledger application - Baking authorizing primitives

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
   Copyright 2021 Ledger
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

#include "baking_auth.h"

#include "apdu.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include "os_cx.h"

#include <string.h>

bool is_valid_level(level_t lvl) {
    return !(lvl & 0xC0000000);
}

void write_high_water_mark(parsed_baking_data_t const *const in) {
    check_null(in);
    if (!is_valid_level(in->level)) {
        THROW(EXC_WRONG_VALUES);
    }
    UPDATE_NVRAM(ram, {
        // If the chain matches the main chain *or* the main chain is not set, then use 'main' HWM.
        high_watermark_t volatile *const dest = select_hwm_by_chain(in->chain_id, ram);
        if ((in->level > dest->highest_level) || (in->round > dest->highest_round)) {
            dest->had_endorsement = false;
            dest->had_preendorsement = false;
        };
        dest->highest_level = CUSTOM_MAX(in->level, dest->highest_level);
        dest->highest_round = in->round;
        dest->had_endorsement |= in->type == BAKING_TYPE_ENDORSEMENT;
        dest->had_preendorsement |= in->type == BAKING_TYPE_PREENDORSEMENT;
        dest->migrated_to_tenderbake |= in->is_tenderbake;
    });
}

void authorize_baking(derivation_type_t const derivation_type,
                      bip32_path_t const *const bip32_path) {
    check_null(bip32_path);
    if (bip32_path->length > NUM_ELEMENTS(N_data.baking_key.bip32_path.components) ||
        bip32_path->length == 0) {
        return;
    }

    UPDATE_NVRAM(ram, {
        ram->baking_key.derivation_type = derivation_type;
        copy_bip32_path(&ram->baking_key.bip32_path, bip32_path);
    });
}

static bool is_level_authorized(parsed_baking_data_t const *const baking_info) {
    check_null(baking_info);
    if (!is_valid_level(baking_info->level)) {
        return false;
    }
    high_watermark_t volatile const *const hwm =
        select_hwm_by_chain(baking_info->chain_id, &N_data);

    if (baking_info->is_tenderbake) {
        return baking_info->level > hwm->highest_level ||

               (baking_info->level == hwm->highest_level &&
                baking_info->round > hwm->highest_round) ||

               // It is ok to sign an endorsement if we have not already signed an endorsement for
               // the level/round
               (baking_info->level == hwm->highest_level &&
                baking_info->round == hwm->highest_round &&
                baking_info->type == BAKING_TYPE_ENDORSEMENT && !hwm->had_endorsement) ||

               // It is ok to sign a preendorsement if we have not already signed neither an
               // endorsement nor a preendorsement for the level/round
               (baking_info->level == hwm->highest_level &&
                baking_info->round == hwm->highest_round &&
                baking_info->type == BAKING_TYPE_PREENDORSEMENT && !hwm->had_endorsement &&
                !hwm->had_preendorsement);

    } else {
        return false;
    }
}

bool is_path_authorized(derivation_type_t const derivation_type,
                        bip32_path_t const *const bip32_path) {
    check_null(bip32_path);
    return derivation_type != 0 && derivation_type == N_data.baking_key.derivation_type &&
           bip32_path->length > 0 &&
           bip32_paths_eq(bip32_path, (const bip32_path_t *) &N_data.baking_key.bip32_path);
}

void guard_baking_authorized(parsed_baking_data_t const *const baking_info,
                             bip32_path_with_curve_t const *const key) {
    check_null(baking_info);
    check_null(key);
    if (!is_path_authorized(key->derivation_type, &key->bip32_path)) {
        THROW(EXC_SECURITY);
    }
    if (!is_level_authorized(baking_info)) {
        THROW(EXC_WRONG_VALUES);
    }
}

struct block_wire {
    uint8_t magic_byte;
    uint32_t chain_id;
    uint32_t level;
    uint8_t proto;
    uint8_t predecessor[32];
    uint64_t timestamp;
    uint8_t validation_pass;
    uint8_t operation_hash[32];
    uint32_t fitness_size;
    // ... beyond this we don't care
} __attribute__((packed));

struct consensus_op_wire {
    uint8_t magic_byte;
    uint32_t chain_id;
    uint8_t branch[32];
    uint8_t tag;
    uint16_t slot;
    uint32_t level;
    uint32_t round;
    uint8_t block_payload_hash[32];
} __attribute__((packed));

/**
 * Fitness =
 *  - tag_size(4)               + tag(1)               +
 *  - level_size(4)             + level(4)             +
 *  - locked_round_size(4)      + locked_round(0|4)    +
 *  - predecessor_round_size(4) + predecessor_round(4) +
 *  - current_round_size(4)     + current_round(4)
 */
#define MINIMUM_FITNESS_SIZE 33  // When 'locked_round' == none
#define MAXIMUM_FITNESS_SIZE 37  // When 'locked_round' != none

#define TENDERBAKE_PROTO_FITNESS_VERSION 2

uint8_t get_proto_version(void const *const fitness) {
    // Each field is preceded by its size (uint32_t).
    // That's why we need to look at `sizeof(uint32_t)` bytes after
    // the start of `fitness` to get to its first field.
    // See:
    // https://gitlab.com/tezos/tezos/-/blob/master/src/proto_alpha/lib_protocol/fitness_repr.ml#L193-201
    // https://gitlab.com/tezos/tezos/-/blob/master/src/lib_base/fitness.ml#L76
    return READ_UNALIGNED_BIG_ENDIAN(uint8_t, fitness + sizeof(uint32_t));
}

bool parse_block(parsed_baking_data_t *const out, void const *const data, size_t const length) {
    if (length < sizeof(struct block_wire) + MINIMUM_FITNESS_SIZE) {
        return false;
    }
    struct block_wire const *const block = data;
    void const *const fitness = data + sizeof(struct block_wire);
    uint8_t proto_version = get_proto_version(fitness);
    if (proto_version != TENDERBAKE_PROTO_FITNESS_VERSION) {
        return false;
    }
    uint32_t fitness_size = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &block->fitness_size);
    if (fitness_size < MINIMUM_FITNESS_SIZE || fitness + fitness_size > data + length ||
        fitness_size > MAXIMUM_FITNESS_SIZE) {
        return false;
    }

    out->chain_id.v = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &block->chain_id);
    out->level = READ_UNALIGNED_BIG_ENDIAN(level_t, &block->level);
    out->type = BAKING_TYPE_BLOCK;
    out->is_tenderbake = true;
    out->round = READ_UNALIGNED_BIG_ENDIAN(uint32_t, (fitness + fitness_size - 4));

    return true;
}

bool parse_consensus_operation(parsed_baking_data_t *const out,
                               void const *const data,
                               size_t const length) {
    if (length < sizeof(struct consensus_op_wire)) {
        return false;
    }
    struct consensus_op_wire const *const op = data;

    out->chain_id.v = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &op->chain_id);
    out->is_tenderbake = true;
    out->level = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &op->level);
    out->round = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &op->round);

    switch (op->tag) {
        case 20:  // preendorsement
            out->type = BAKING_TYPE_PREENDORSEMENT;
            break;
        case 21:  // endorsement
        case 23:  // endorsement + DAL
            out->type = BAKING_TYPE_ENDORSEMENT;
            break;
        default:
            return false;
    }
    return true;
}

bool parse_baking_data(parsed_baking_data_t *const out,
                       void const *const data,
                       size_t const length) {
    switch (get_magic_byte(data, length)) {
        case MAGIC_BYTE_PREENDORSEMENT:
        case MAGIC_BYTE_ENDORSEMENT:
            return parse_consensus_operation(out, data, length);
        case MAGIC_BYTE_BLOCK:
            return parse_block(out, data, length);
        case MAGIC_BYTE_INVALID:
        default:
            return false;
    }
}
