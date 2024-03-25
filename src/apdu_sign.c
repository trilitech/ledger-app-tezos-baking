/* Tezos Ledger application - Sign APDU instruction handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
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

#include "apdu_sign.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "to_string.h"
#include "ui.h"
#include "ui_delegation.h"

#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

#define B2B_BLOCKBYTES 128u  /// blake2b hash size

static int perform_signature(bool const send_hash);

/**
 * @brief Initializes the blake2b state if it is not
 *
 * @param state: blake2b state
 */
static inline void conditional_init_hash_state(blake2b_hash_state_t *const state) {
    check_null(state);
    if (!state->initialized) {
        // cx_blake2b_init takes size in bits.
        CX_THROW(cx_blake2b_init_no_throw(&state->state, SIGN_HASH_SIZE * 8u));
        state->initialized = true;
    }
}

/**
 * @brief Hashes incrementally a buffer using a blake2b state
 *
 *        The hash remains in the buffer, the buffer content length is
 *        updated and the blake2b state is also updated
 *
 * @param buff: buffer
 * @param buff_size: buffer size
 * @param buff_length: buffer content length
 * @param state: blake2b state
 */
static void blake2b_incremental_hash(uint8_t *const buff,
                                     size_t const buff_size,
                                     size_t *const buff_length,
                                     blake2b_hash_state_t *const state) {
    check_null(buff);
    check_null(buff_length);
    check_null(state);

    uint8_t *current = buff;
    while (*buff_length > B2B_BLOCKBYTES) {
        if ((current - buff) > (int) buff_size) {
            THROW(EXC_MEMORY_ERROR);
        }
        conditional_init_hash_state(state);
        CX_THROW(
            cx_hash_no_throw((cx_hash_t *) &state->state, 0, current, B2B_BLOCKBYTES, NULL, 0));
        *buff_length -= B2B_BLOCKBYTES;
        current += B2B_BLOCKBYTES;
    }
    // TODO use circular buffer at some point
    memmove(buff, current, *buff_length);
}

/**
 * @brief Finalizes the hashes of a buffer using a blake2b state
 *
 *        The buffer is modified and the buffer content length is
 *        updated
 *
 *        The final hash is stored in the ouput, its size is also
 *         stored and the blake2b state is updated
 *
 * @param out: output buffer
 * @param out_size: output size
 * @param buff: buffer
 * @param buff_size: buffer size
 * @param buff_length: buffer content length
 * @param state: blake2b state
 */
static void blake2b_finish_hash(uint8_t *const out,
                                size_t const out_size,
                                uint8_t *const buff,
                                size_t const buff_size,
                                size_t *const buff_length,
                                blake2b_hash_state_t *const state) {
    check_null(out);
    check_null(buff);
    check_null(buff_length);
    check_null(state);

    conditional_init_hash_state(state);
    blake2b_incremental_hash(buff, buff_size, buff_length, state);
    CX_THROW(
        cx_hash_no_throw((cx_hash_t *) &state->state, CX_LAST, buff, *buff_length, out, out_size));
}

/**
 * @brief Allows to clear all data related to signature
 *
 */
static inline void clear_data(void) {
    memset(&G, 0, sizeof(G));
}

/**
 * @brief Sends asynchronously the signature of the read message
 *
 * @return true
 */
static bool sign_without_hash_ok(void) {
    perform_signature(false);
    return true;
}

/**
 * @brief Sends asynchronously the signature of the read message
 *        preceded by its hash
 *
 * @return true
 */
static bool sign_with_hash_ok(void) {
    perform_signature(true);
    return true;
}

/**
 * @brief Rejects the signature
 *
 *        Makes sure to keep no trace of the signature
 *
 * @return true: to return to idle without showing a reject screen
 */
static bool sign_reject(void) {
    clear_data();
    reject();
    return true;
}

/// Magic byte values
/// See: https://tezos.gitlab.io/user/key-management.html#signer-requests
#define MAGIC_BYTE_UNSAFE_OP      0x03u  /// magic byte of an operation
#define MAGIC_BYTE_BLOCK          0x11u  /// magic byte of a block
#define MAGIC_BYTE_PREATTESTATION 0x12u  /// magic byte of a pre-attestation
#define MAGIC_BYTE_ATTESTATION    0x13u  /// magic byte of an attestation

/**
 * @brief Carries out final checks before signing
 *
 * @param send_hash: if the message hash is requested
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
static int baking_sign_complete(bool const send_hash) {
    int result = 0;
    switch (G.magic_byte) {
        case MAGIC_BYTE_BLOCK:
        case MAGIC_BYTE_PREATTESTATION:
        case MAGIC_BYTE_ATTESTATION:
            guard_baking_authorized(&G.parsed_baking_data, &global.path_with_curve);
            result = perform_signature(send_hash);
#ifdef HAVE_BAGL
            update_baking_idle_screens();
#endif
            break;

        case MAGIC_BYTE_UNSAFE_OP: {
            if (!G.maybe_ops.is_valid) {
                PARSE_ERROR();
            }

            switch (G.maybe_ops.v.operation.tag) {
                case OPERATION_TAG_DELEGATION:
                    // Must be self-delegation signed by the *authorized* baking key
                    if (bip32_path_with_curve_eq(&global.path_with_curve, &N_data.baking_key) &&
                        // ops->signing is generated from G.bip32_path and G.curve
                        COMPARE(G.maybe_ops.v.operation.source, G.maybe_ops.v.signing) == 0 &&
                        COMPARE(G.maybe_ops.v.operation.destination, G.maybe_ops.v.signing) == 0) {
                        ui_callback_t const ok_c =
                            send_hash ? sign_with_hash_ok : sign_without_hash_ok;
                        result = prompt_delegation(ok_c, sign_reject);
                    } else {
                        THROW(EXC_SECURITY);
                    }
                    break;
                case OPERATION_TAG_REVEAL:
                case OPERATION_TAG_NONE:
                    // Reveal cases
                    if (bip32_path_with_curve_eq(&global.path_with_curve, &N_data.baking_key) &&
                        // ops->signing is generated from G.bip32_path and G.curve
                        COMPARE(G.maybe_ops.v.operation.source, G.maybe_ops.v.signing) == 0) {
                        result = perform_signature(send_hash);
                    } else {
                        THROW(EXC_SECURITY);
                    }
                    break;
                default:
                    THROW(EXC_SECURITY);
            }
            break;
        }
        default:
            PARSE_ERROR();
    }
    return result;
}

/**
 * Cdata:
 *   + Bip32 path: signing key path
 */
int select_signing_key(buffer_t *cdata, derivation_type_t derivation_type) {
    check_null(cdata);

    clear_data();

    if (!read_bip32_path(cdata, &global.path_with_curve.bip32_path)) {
        THROW(EXC_WRONG_VALUES);
    }

    if (cdata->size != cdata->offset) {
        THROW(EXC_WRONG_LENGTH);
    }

    global.path_with_curve.derivation_type = derivation_type;

    return io_send_sw(SW_OK);
}

/**
 * Cdata:
 *   + (max-size) uint8 *: message
 */
int handle_sign(buffer_t *cdata, bool last, bool with_hash) {
    check_null(cdata);

    if (global.path_with_curve.bip32_path.length == 0u) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    // Guard against overflow
    if (G.packet_index >= 0xFFu) {
        PARSE_ERROR();
    }
    G.packet_index++;

    if (G.packet_index != 1u) {
        PARSE_ERROR();  // Only parse a single packet when baking
    }

    if (!buffer_read_u8(cdata, &G.magic_byte)) {
        PARSE_ERROR();
    }

    switch (G.magic_byte) {
        case MAGIC_BYTE_PREATTESTATION:
        case MAGIC_BYTE_ATTESTATION:
            if (!parse_consensus_operation(cdata, &G.parsed_baking_data)) {
                PARSE_ERROR();
            }
            break;
        case MAGIC_BYTE_BLOCK:
            if (!parse_block(cdata, &G.parsed_baking_data)) {
                PARSE_ERROR();
            }
            break;
        case MAGIC_BYTE_UNSAFE_OP: {
            // Parse the operation. It will be verified in `baking_sign_complete`.
            tz_exc exc = parse_operations(cdata, &G.maybe_ops.v, &global.path_with_curve);

            if (exc != SW_OK) {
                THROW(exc);
            }
            break;
        }
        default:
            PARSE_ERROR();
    }

    // Hash contents of *previous* message (which may be empty).
    blake2b_incremental_hash(G.message_data,
                             sizeof(G.message_data),
                             &G.message_data_length,
                             &G.hash_state);

    if (!buffer_seek_set(cdata, 0) ||
        !buffer_move(cdata,
                     G.message_data + G.message_data_length,
                     sizeof(G.message_data) - G.message_data_length)) {
        PARSE_ERROR();
    }
    G.message_data_length += cdata->size;

    if (last) {
        // Hash contents of *this* message and then get the final hash value.
        blake2b_incremental_hash(G.message_data,
                                 sizeof(G.message_data),
                                 &G.message_data_length,
                                 &G.hash_state);
        blake2b_finish_hash(G.final_hash,
                            sizeof(G.final_hash),
                            G.message_data,
                            sizeof(G.message_data),
                            &G.message_data_length,
                            &G.hash_state);

        G.maybe_ops.is_valid = parse_operations_final(&G.parse_state, &G.maybe_ops.v);

        return baking_sign_complete(with_hash);
    } else {
        return io_send_sw(SW_OK);
    }
}

/**
 * @brief Perfoms the signature of the read message
 *
 *        Fills apdu response with the signature
 *
 *        Precedes the signature with the message hash if requested
 *
 * @param send_hash: if the message hash is requested
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
static int perform_signature(bool const send_hash) {
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        THROW(EXC_SECURITY);
    }

    write_high_water_mark(&G.parsed_baking_data);

    uint8_t resp[SIGN_HASH_SIZE + MAX_SIGNATURE_SIZE] = {0};
    size_t offset = 0;

    if (send_hash) {
        memcpy(resp + offset, G.final_hash, sizeof(G.final_hash));
        offset += sizeof(G.final_hash);
    }

    size_t signature_size = MAX_SIGNATURE_SIZE;

    CX_THROW(sign(resp + offset,
                  &signature_size,
                  &global.path_with_curve,
                  G.final_hash,
                  sizeof(G.final_hash)));

    offset += signature_size;

    clear_data();

    return io_send_response_pointer(resp, offset, SW_OK);
}
