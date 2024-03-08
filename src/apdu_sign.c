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
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

#define B2B_BLOCKBYTES 128

static inline void conditional_init_hash_state(blake2b_hash_state_t *const state) {
    check_null(state);
    if (!state->initialized) {
        // cx_blake2b_init takes size in bits.
        CX_THROW(cx_blake2b_init_no_throw(&state->state, SIGN_HASH_SIZE * 8));
        state->initialized = true;
    }
}

static void blake2b_incremental_hash(
    /*in/out*/ uint8_t *const out,
    size_t const out_size,
    /*in/out*/ size_t *const out_length,
    /*in/out*/ blake2b_hash_state_t *const state) {
    check_null(out);
    check_null(out_length);
    check_null(state);

    uint8_t *current = out;
    while (*out_length > B2B_BLOCKBYTES) {
        if (current - out > (int) out_size) {
            THROW(EXC_MEMORY_ERROR);
        }
        conditional_init_hash_state(state);
        CX_THROW(
            cx_hash_no_throw((cx_hash_t *) &state->state, 0, current, B2B_BLOCKBYTES, NULL, 0));
        *out_length -= B2B_BLOCKBYTES;
        current += B2B_BLOCKBYTES;
    }
    // TODO use circular buffer at some point
    memmove(out, current, *out_length);
}

static void blake2b_finish_hash(
    /*out*/ uint8_t *const out,
    size_t const out_size,
    /*in/out*/ uint8_t *const buff,
    size_t const buff_size,
    /*in/out*/ size_t *const buff_length,
    /*in/out*/ blake2b_hash_state_t *const state) {
    check_null(out);
    check_null(buff);
    check_null(buff_length);
    check_null(state);

    conditional_init_hash_state(state);
    blake2b_incremental_hash(buff, buff_size, buff_length, state);
    CX_THROW(
        cx_hash_no_throw((cx_hash_t *) &state->state, CX_LAST, buff, *buff_length, out, out_size));
}

static inline void clear_data(void) {
    memset(&G, 0, sizeof(G));
}

static bool sign_without_hash_ok(void) {
    delayed_send(perform_signature(true, false));
    return true;
}

static bool sign_with_hash_ok(void) {
    delayed_send(perform_signature(true, true));
    return true;
}

static bool sign_reject(void) {
    clear_data();
    delay_reject();
    return true;  // Return to idle
}

size_t baking_sign_complete(bool const send_hash, volatile uint32_t *flags) {
    size_t result = 0;
    switch (G.magic_byte) {
        case MAGIC_BYTE_BLOCK:
        case MAGIC_BYTE_PREENDORSEMENT:
        case MAGIC_BYTE_ENDORSEMENT:
            guard_baking_authorized(&G.parsed_baking_data, &global.path_with_curve);
            result = perform_signature(true, send_hash);
            ux_empty_screen();
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
                        COMPARE(&G.maybe_ops.v.operation.source, &G.maybe_ops.v.signing) == 0 &&
                        COMPARE(&G.maybe_ops.v.operation.destination, &G.maybe_ops.v.signing) ==
                            0) {
                        ui_callback_t const ok_c =
                            send_hash ? sign_with_hash_ok : sign_without_hash_ok;
                        prompt_register_delegate(ok_c, sign_reject);
                        *flags = IO_ASYNCH_REPLY;
                        result = 0;
                    } else {
                        THROW(EXC_SECURITY);
                    }
                    break;
                case OPERATION_TAG_REVEAL:
                case OPERATION_TAG_NONE:
                    // Reveal cases
                    if (bip32_path_with_curve_eq(&global.path_with_curve, &N_data.baking_key) &&
                        // ops->signing is generated from G.bip32_path and G.curve
                        COMPARE(&G.maybe_ops.v.operation.source, &G.maybe_ops.v.signing) == 0) {
                        result = perform_signature(true, send_hash);
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

#define P1_FIRST          0x00
#define P1_NEXT           0x01
#define P1_HASH_ONLY_NEXT 0x03  // You only need it once
#define P1_LAST_MARKER    0x80

static uint8_t get_magic_byte_or_throw(uint8_t const *const buff, size_t const buff_size) {
    uint8_t const magic_byte = get_magic_byte(buff, buff_size);
    switch (magic_byte) {
        case MAGIC_BYTE_BLOCK:
        case MAGIC_BYTE_PREENDORSEMENT:
        case MAGIC_BYTE_ENDORSEMENT:
        case MAGIC_BYTE_UNSAFE_OP:  // Only for self-delegations
            return magic_byte;

        default:
            PARSE_ERROR();
    }
}

static size_t handle_apdu(bool const enable_hashing,
                          bool const enable_parsing,
                          uint8_t const instruction,
                          volatile uint32_t *flags) {
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        THROW(EXC_SECURITY);
    }
    uint8_t *const buff = &G_io_apdu_buffer[OFFSET_CDATA];
    uint8_t const p1 = G_io_apdu_buffer[OFFSET_P1];
    uint8_t const buff_size = G_io_apdu_buffer[OFFSET_LC];
    if (buff_size > MAX_APDU_SIZE) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    bool last = (p1 & P1_LAST_MARKER) != 0;
    switch (p1 & ~P1_LAST_MARKER) {
        case P1_FIRST:
            clear_data();
            read_bip32_path(&global.path_with_curve.bip32_path, buff, buff_size);
            global.path_with_curve.derivation_type =
                parse_derivation_type(G_io_apdu_buffer[OFFSET_CURVE]);
            return finalize_successful_send(0);
        case P1_NEXT:
            if (global.path_with_curve.bip32_path.length == 0) {
                THROW(EXC_WRONG_LENGTH_FOR_INS);
            }

            // Guard against overflow
            if (G.packet_index >= 0xFF) {
                PARSE_ERROR();
            }
            G.packet_index++;

            break;
        default:
            THROW(EXC_WRONG_PARAM);
    }

    if (enable_parsing) {
        if (G.packet_index != 1) {
            PARSE_ERROR();  // Only parse a single packet when baking
        }

        G.magic_byte = get_magic_byte_or_throw(buff, buff_size);
        if (G.magic_byte == MAGIC_BYTE_UNSAFE_OP) {
            // Parse the operation. It will be verified in `baking_sign_complete`.
            G.maybe_ops.is_valid = parse_operations(&G.maybe_ops.v,
                                                    buff,
                                                    buff_size,
                                                    global.path_with_curve.derivation_type,
                                                    &global.path_with_curve.bip32_path);
        } else {
            // This should be a baking operation so parse it.
            if (!parse_baking_data(&G.parsed_baking_data, buff, buff_size)) {
                PARSE_ERROR();
            }
        }
    }

    if (enable_hashing) {
        // Hash contents of *previous* message (which may be empty).
        blake2b_incremental_hash(G.message_data,
                                 sizeof(G.message_data),
                                 &G.message_data_length,
                                 &G.hash_state);
    }

    if (G.message_data_length + buff_size > sizeof(G.message_data)) {
        PARSE_ERROR();
    }

    memmove(G.message_data + G.message_data_length, buff, buff_size);
    G.message_data_length += buff_size;

    if (last) {
        if (enable_hashing) {
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
        }

        G.maybe_ops.is_valid = parse_operations_final(&G.parse_state, &G.maybe_ops.v);

        return baking_sign_complete(instruction == INS_SIGN_WITH_HASH, flags);
    } else {
        return finalize_successful_send(0);
    }
}

size_t handle_apdu_sign(uint8_t instruction, volatile uint32_t *flags) {
    bool const enable_hashing = instruction != INS_SIGN_UNSAFE;
    bool const enable_parsing = enable_hashing;
    return handle_apdu(enable_hashing, enable_parsing, instruction, flags);
}

size_t handle_apdu_sign_with_hash(uint8_t instruction, volatile uint32_t *flags) {
    bool const enable_hashing = true;
    bool const enable_parsing = true;
    return handle_apdu(enable_hashing, enable_parsing, instruction, flags);
}

int perform_signature(bool const on_hash, bool const send_hash) {
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        THROW(EXC_SECURITY);
    }

    write_high_water_mark(&G.parsed_baking_data);
    size_t tx = 0;
    if (send_hash && on_hash) {
        memcpy(&G_io_apdu_buffer[tx], G.final_hash, sizeof(G.final_hash));
        tx += sizeof(G.final_hash);
    }

    uint8_t const *const data = on_hash ? G.final_hash : G.message_data;
    size_t const data_length = on_hash ? sizeof(G.final_hash) : G.message_data_length;

    key_pair_t key_pair = {0};
    size_t signature_size = 0;

    int error = generate_key_pair(&key_pair,
                                  global.path_with_curve.derivation_type,
                                  &global.path_with_curve.bip32_path);
    if (error) {
        THROW(EXC_WRONG_VALUES);
    }

    error = 0;
    BEGIN_TRY {
        TRY {
            signature_size = sign(&G_io_apdu_buffer[tx],
                                  MAX_SIGNATURE_SIZE,
                                  global.path_with_curve.derivation_type,
                                  &key_pair,
                                  data,
                                  data_length);
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            memset(&key_pair, 0, sizeof(key_pair));
        }
    }
    END_TRY;

    if (error) {
        THROW(error);
    }

    tx += signature_size;

    clear_data();
    return finalize_successful_send(tx);
}
