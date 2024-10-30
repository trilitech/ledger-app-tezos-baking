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

#define B2B_BLOCKBYTES 128u  /// blake2b hash size

static int perform_signature(bool const send_hash);

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

/**
 * @brief Carries out final checks before signing
 *
 * @param send_hash: if the message hash is requested
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
static int baking_sign_complete(bool const send_hash) {
    tz_exc exc = SW_OK;
    int result = 0;
    switch (G.magic_byte) {
        case MAGIC_BYTE_BLOCK:
        case MAGIC_BYTE_PREATTESTATION:
        case MAGIC_BYTE_ATTESTATION:
            TZ_CHECK(guard_baking_authorized(&G.parsed_baking_data, &global.path_with_curve));
#ifdef TARGET_NANOS
            // To be efficient, the signing needs a low-cost display
            ux_set_low_cost_display_mode(true);
#endif
            result = perform_signature(send_hash);
#ifdef HAVE_BAGL
            // Ignore calculation errors
            calculate_idle_screen_hwm();
            // The HWM screen is not updated to avoid slowing down the
            // application. Updating the HWM screen may slow down the
            // next signing.
#endif
            break;

        case MAGIC_BYTE_UNSAFE_OP: {
            TZ_ASSERT(G.maybe_ops.is_valid, EXC_PARSE_ERROR);

            switch (G.maybe_ops.v.operation.tag) {
                case OPERATION_TAG_DELEGATION:
                    // Must be self-delegation signed by the *authorized* baking key
                    TZ_ASSERT(
                        bip32_path_with_curve_eq(&global.path_with_curve, &g_hwm.baking_key) &&
                            // ops->signing is generated from G.bip32_path and G.curve
                            (COMPARE(G.maybe_ops.v.operation.source, G.maybe_ops.v.signing) == 0) &&
                            (COMPARE(G.maybe_ops.v.operation.destination, G.maybe_ops.v.signing) ==
                             0),
                        EXC_SECURITY);
                    ui_callback_t const ok_c = send_hash ? sign_with_hash_ok : sign_without_hash_ok;
                    result = prompt_delegation(ok_c, sign_reject);
                    break;
                case OPERATION_TAG_REVEAL:
                case OPERATION_TAG_NONE:
                    // Reveal cases
                    TZ_ASSERT(
                        bip32_path_with_curve_eq(&global.path_with_curve, &g_hwm.baking_key) &&
                            // ops->signing is generated from G.bip32_path and G.curve
                            (COMPARE(G.maybe_ops.v.operation.source, G.maybe_ops.v.signing) == 0),
                        EXC_SECURITY);
                    result = perform_signature(send_hash);
                    break;
                default:
                    TZ_FAIL(EXC_SECURITY);
            }
            break;
        }
        default:
            TZ_FAIL(EXC_PARSE_ERROR);
    }
    return result;

end:
    return io_send_apdu_err(exc);
}

/**
 * Cdata:
 *   + Bip32 path: signing key path
 */
int select_signing_key(buffer_t *cdata, derivation_type_t derivation_type) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(cdata);

    clear_data();

    TZ_ASSERT(read_bip32_path(cdata, &global.path_with_curve.bip32_path), EXC_WRONG_VALUES);

    TZ_ASSERT(cdata->size == cdata->offset, EXC_WRONG_LENGTH);

    global.path_with_curve.derivation_type = derivation_type;

    return io_send_sw(SW_OK);

end:
    return io_send_apdu_err(exc);
}

/**
 * Cdata:
 *   + (max-size) uint8 *: message
 */
int handle_sign(buffer_t *cdata, const bool last, const bool with_hash) {
    tz_exc exc = SW_OK;
    cx_err_t error = CX_OK;

    TZ_ASSERT_NOT_NULL(cdata);

    TZ_ASSERT(global.path_with_curve.bip32_path.length != 0u, EXC_WRONG_LENGTH_FOR_INS);

    // Guard against overflow
    TZ_ASSERT(G.packet_index < 0xFFu, EXC_PARSE_ERROR);
    G.packet_index++;

    // Only parse a single packet when baking
    TZ_ASSERT(G.packet_index == 1u, EXC_PARSE_ERROR);
    if (G.packet_index == 1u) {
        CX_CHECK(cx_hash_init_ex((cx_hash_t *) &G.hash_state.state, CX_BLAKE2B, SIGN_HASH_SIZE));
    }

    TZ_ASSERT(buffer_read_u8(cdata, &G.magic_byte), EXC_PARSE_ERROR);
    bool is_attestation = false;

    switch (G.magic_byte) {
        case MAGIC_BYTE_PREATTESTATION:
            is_attestation = false;
            TZ_ASSERT(parse_consensus_operation(cdata, &G.parsed_baking_data, is_attestation),
                      EXC_PARSE_ERROR);
            break;
        case MAGIC_BYTE_ATTESTATION:
            is_attestation = true;
            TZ_ASSERT(parse_consensus_operation(cdata, &G.parsed_baking_data, is_attestation),
                      EXC_PARSE_ERROR);
            break;
        case MAGIC_BYTE_BLOCK:
            TZ_ASSERT(parse_block(cdata, &G.parsed_baking_data), EXC_PARSE_ERROR);
            break;
        case MAGIC_BYTE_UNSAFE_OP:
            // Parse the operation. It will be verified in `baking_sign_complete`.
            TZ_CHECK(parse_operations(cdata, &G.maybe_ops.v, &global.path_with_curve));
            break;
        default:
            TZ_FAIL(EXC_PARSE_ERROR);
    }

    CX_CHECK(
        cx_hash_no_throw((cx_hash_t *) &G.hash_state.state, 0, cdata->ptr, cdata->size, NULL, 0));

#ifndef TARGET_NANOS
    memmove(G.message, cdata->ptr, cdata->size);
    G.message_len = cdata->size;
#endif

    if (last) {
        CX_CHECK(cx_hash_no_throw((cx_hash_t *) &G.hash_state.state,
                                  CX_LAST,
                                  NULL,
                                  0,
                                  G.final_hash,
                                  sizeof(G.final_hash)));

        G.maybe_ops.is_valid = parse_operations_final(&G.parse_state, &G.maybe_ops.v);

        return baking_sign_complete(with_hash);
    } else {
        return io_send_sw(SW_OK);
    }

end:
    TZ_CONVERT_CX();
    return io_send_apdu_err(exc);
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
    tz_exc exc = SW_OK;
    cx_err_t error = CX_OK;

    TZ_ASSERT(os_global_pin_is_validated() == BOLOS_UX_OK, EXC_SECURITY);

    TZ_CHECK(write_high_water_mark(&G.parsed_baking_data));

    uint8_t resp[SIGN_HASH_SIZE + MAX_SIGNATURE_SIZE] = {0};
    size_t offset = 0;

    uint8_t *message = G.final_hash;
    size_t message_len = sizeof(G.final_hash);

#ifndef TARGET_NANOS
    // The BLS signature uses its own hash function.
    // The entire message must be stored in `G.message`.
    if (global.path_with_curve.derivation_type == DERIVATION_TYPE_BLS12_381) {
        message = G.message;
        message_len = G.message_len;
    }
#endif

    if (send_hash) {
        memcpy(resp + offset, G.final_hash, sizeof(G.final_hash));
        offset += sizeof(G.final_hash);
    }

    size_t signature_size = MAX_SIGNATURE_SIZE;

    CX_CHECK(sign(resp + offset, &signature_size, &global.path_with_curve, message, message_len));

    offset += signature_size;

    clear_data();

    return io_send_response_pointer(resp, offset, SW_OK);

end:
    TZ_CONVERT_CX();
    return io_send_apdu_err(exc);
}
