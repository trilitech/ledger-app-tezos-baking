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
#include "to_string.h"
#include "ui.h"

#include "os_cx.h"

#include <string.h>

bool is_valid_level(level_t lvl) {
    return !(lvl & 0xC0000000);
}

tz_exc write_high_water_mark(parsed_baking_data_t const *const in) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(in);

    TZ_ASSERT(is_valid_level(in->level), EXC_WRONG_VALUES);

    UPDATE_NVRAM(ram, {
        // If the chain matches the main chain *or* the main chain is not set, then use 'main' HWM.
        high_watermark_t volatile *const dest = select_hwm_by_chain(in->chain_id, ram);
        TZ_ASSERT_NOT_NULL(dest);

        if ((in->level > dest->highest_level) || (in->round > dest->highest_round)) {
            dest->had_attestation = false;
            dest->had_preattestation = false;
        };
        dest->highest_level = CUSTOM_MAX(in->level, dest->highest_level);
        dest->highest_round = in->round;
        dest->had_attestation |= in->type == BAKING_TYPE_ATTESTATION;
        dest->had_preattestation |= in->type == BAKING_TYPE_PREATTESTATION;
        dest->migrated_to_tenderbake |= in->is_tenderbake;
    });

end:
    return exc;
}

tz_exc authorize_baking(derivation_type_t const derivation_type,
                        bip32_path_t const *const bip32_path) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(bip32_path);

    TZ_ASSERT(bip32_path->length <= NUM_ELEMENTS(N_data.baking_key.bip32_path.components),
              EXC_WRONG_LENGTH);

    if (bip32_path->length != 0u) {
        UPDATE_NVRAM(ram, {
            ram->baking_key.derivation_type = derivation_type;
            copy_bip32_path(&ram->baking_key.bip32_path, bip32_path);
        });
    }

end:
    return exc;
}

/**
 * @brief Checks if a baking info pass all checks
 *
 *        See `doc/signing.md#checks`
 *
 * @param baking_info: baking info
 * @return bool: return true if it has passed checks
 */
static bool is_level_authorized(parsed_baking_data_t const *const baking_info) {
    if (baking_info == NULL) {
        return false;
    }

    if (!is_valid_level(baking_info->level)) {
        return false;
    }

    high_watermark_t volatile const *const hwm =
        select_hwm_by_chain(baking_info->chain_id, &N_data);
    if (hwm == NULL) {
        return false;
    }

    if (!baking_info->is_tenderbake) {
        return false;
    }

    return (baking_info->level > hwm->highest_level) ||

           ((baking_info->level == hwm->highest_level) &&
            (baking_info->round > hwm->highest_round)) ||

           // It is ok to sign an attestation if we have not already signed an attestation for
           // the level/round
           ((baking_info->level == hwm->highest_level) &&
            (baking_info->round == hwm->highest_round) &&
            (baking_info->type == BAKING_TYPE_ATTESTATION) && !hwm->had_attestation) ||

           // It is ok to sign a preattestation if we have not already signed neither an
           // attestation nor a preattestation for the level/round
           ((baking_info->level == hwm->highest_level) &&
            (baking_info->round == hwm->highest_round) &&
            (baking_info->type == BAKING_TYPE_PREATTESTATION) && !hwm->had_attestation &&
            !hwm->had_preattestation);
}

/**
 * @brief Checks if a key pass the checks
 *
 * @param derivation_type: curve of the key
 * @param bip32_path: bip32 path of the key
 * @return bool: return true if it has passed checks
 */
static bool is_path_authorized(derivation_type_t const derivation_type,
                               bip32_path_t const *const bip32_path) {
    return (bip32_path != NULL) && (derivation_type != DERIVATION_TYPE_UNSET) &&
           (derivation_type == N_data.baking_key.derivation_type) && (bip32_path->length != 0u) &&
           bip32_paths_eq(bip32_path, (const bip32_path_t *) &N_data.baking_key.bip32_path);
}

tz_exc guard_baking_authorized(parsed_baking_data_t const *const baking_info,
                               bip32_path_with_curve_t const *const key) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(baking_info);
    TZ_ASSERT_NOT_NULL(key);
    TZ_ASSERT(is_path_authorized(key->derivation_type, &key->bip32_path), EXC_SECURITY);
    TZ_ASSERT(is_level_authorized(baking_info), EXC_WRONG_VALUES);

end:
    return exc;
}

#define MINIMUM_FITNESS_SIZE 33u  // When 'locked_round' == none
#define MAXIMUM_FITNESS_SIZE 37u  // When 'locked_round' != none

#define TENDERBAKE_PROTO_FITNESS_VERSION 2u

/**
 * Data:
 *   + (4 bytes)  uint32:  chain id of the block
 *   + (4 bytes)  uint32:  level of the block
 *   + (1 byte)   uint8:   protocol number
 *   + (32 bytes) uint8 *: hash of the preceding block
 *   + (8 bytes)  uint64:  timestamp at which the block have been created
 *   + (1 bytes)  uint8:   number of validation passes
 *   + (32 bytes) uint8 *: hash of the operations
 *   + Fitness:
 *     + (4 bytes)     uint32: size of the fitness
 *     + list:
 *       + (4 bytes) uint32: component-size
 *       + (component-size bytes): component
 *   + (max-size) ignored
 *
 * Tenderbake fitness components:
 *   + (1 byte)    uint8:       tag (= 2)
 *   + (4 bytes)   uint32:      level
 *   + (0|4 bytes) None|uint32: locked_round
 *   + (4 bytes)   uint32:      predecessor_round
 *   + (4 bytes)   uint32:      current_round
 */
bool parse_block(buffer_t *buf, parsed_baking_data_t *const out) {
    uint8_t tag;
    uint32_t size;

    if (!buffer_read_u32(buf, &out->chain_id.v, BE) ||  // chain id
        !buffer_read_u32(buf, &out->level, BE) ||       // level
        !buffer_seek_cur(buf, 1) ||                     // ignore protocol number
        !buffer_seek_cur(buf, 32) ||                    // ignore predecessor hash
        !buffer_seek_cur(buf, 8) ||                     // ignore timestamp
        !buffer_seek_cur(buf, 1) ||                     // ignore validation_pass
        !buffer_seek_cur(buf, 32)                       // ignore hash
    ) {
        return false;
    }

    // Fitness
    if (!buffer_read_u32(buf, &size, BE) ||                                    // fitness size
        ((size != MINIMUM_FITNESS_SIZE) && (size != MAXIMUM_FITNESS_SIZE)) ||  // fitness size check
        !buffer_read_u32(buf, &size, BE) || (size != 1u) ||                    // tag size
        !buffer_read_u8(buf, &tag) || (tag != TENDERBAKE_PROTO_FITNESS_VERSION) ||  // tag
        !buffer_read_u32(buf, &size, BE) || !buffer_seek_cur(buf, size) ||          // ignore level
        !buffer_read_u32(buf, &size, BE) || !buffer_seek_cur(buf, size) ||  // ignore locked_round
        !buffer_read_u32(buf, &size, BE) || !buffer_seek_cur(buf, size) ||  // ignore pred_round
        !buffer_read_u32(buf, &size, BE) || (size != 4u) ||                 // current_round size
        !buffer_read_u32(buf, &out->round, BE)                              // current_round
    ) {
        return false;
    }
    out->type = BAKING_TYPE_BLOCK;
    out->is_tenderbake = true;

    return true;
}

#define TAG_PREATTESTATION  20
#define TAG_ATTESTATION     21
#define TAG_ATTESTATION_DAL 23

/**
 * Data:
 *   + (4 bytes)  uint32:  chain id of the block
 *   + (32 bytes) uint8 *: block branch
 *   + (1 byte)   uint8:   operation tag
 *   + (2 bytes)  uint16:  first slot of the baker
 *   + (4 bytes)  uint32:  level of the related block
 *   + (4 bytes)  uint32:  round of the related block
 *   + (32 bytes) uint8 *: hash of the related block
 */
bool parse_consensus_operation(buffer_t *buf, parsed_baking_data_t *const out) {
    uint8_t tag;

    if (!buffer_read_u32(buf, &out->chain_id.v, BE) ||   // chain id
        !buffer_seek_cur(buf, 32u * sizeof(uint8_t)) ||  // ignore branch
        !buffer_read_u8(buf, &tag) ||                    // tag
        !buffer_seek_cur(buf, sizeof(uint16_t)) ||       // ignore slot
        !buffer_read_u32(buf, &out->level, BE) ||        // level
        !buffer_read_u32(buf, &out->round, BE) ||        // round
        !buffer_seek_cur(buf, 32u * sizeof(uint8_t))     // ignore hash
    ) {
        return false;
    }

    switch (tag) {
        case TAG_PREATTESTATION:
            out->type = BAKING_TYPE_PREATTESTATION;
            break;
        case TAG_ATTESTATION:
        case TAG_ATTESTATION_DAL:
            out->type = BAKING_TYPE_ATTESTATION;
            break;
        default:
            return false;
    }

    out->is_tenderbake = true;

    return true;
}
