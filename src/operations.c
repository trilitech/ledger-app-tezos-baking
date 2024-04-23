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

#include "operations.h"

#include "apdu.h"
#include "globals.h"
#include "memory.h"
#include "to_string.h"
#include "ui.h"

#include <stdint.h>
#include <string.h>

/// Parser result
typedef enum {
    PARSER_DONE,      // parsing has ended successfully
    PARSER_CONTINUE,  // parsing did not finish
    PARSER_ERROR      // parsing fail
} tz_parser_result;

// Checks that parsing  has ended successfully
#define PARSER_CHECK(call)        \
    do {                          \
        res = (call);             \
        if (res != PARSER_DONE) { \
            goto end;             \
        }                         \
    } while (0)

// Fail parsing
#define PARSER_FAIL()       \
    do {                    \
        res = PARSER_ERROR; \
        goto end;           \
    } while (0)

// Asserts a condition
#define PARSER_ASSERT(cond) \
    do {                    \
        if (!(cond)) {      \
            PARSER_FAIL();  \
        }                   \
    } while (0)

#define STEP_HARD_FAIL -2

/// Conversion/check functions

/**
 * @brief Get signature_type from a raw signature_type
 *
 * @param raw_signature_type: raw signature_type
 * @param signature_type: signature_type result
 * @return tz_parser_result: result of the parsing
 */
static tz_parser_result parse_raw_tezos_header_signature_type(
    raw_tezos_header_signature_type_t const *const raw_signature_type,
    signature_type_t *signature_type) {
    tz_parser_result res = PARSER_CONTINUE;
    signature_type_t signature_type_result;

    PARSER_ASSERT(raw_signature_type != NULL);

    switch (raw_signature_type->v) {
        case 0:
            signature_type_result = SIGNATURE_TYPE_ED25519;
            break;
        case 1:
            signature_type_result = SIGNATURE_TYPE_SECP256K1;
            break;
        case 2:
            signature_type_result = SIGNATURE_TYPE_SECP256R1;
            break;
        default:
            PARSER_FAIL();
    }

    *signature_type = signature_type_result;
    res = PARSER_DONE;

end:
    return res;
}

/**
 * @brief Extracts a compressed_pubkey and a contract from a key
 *
 * @param compressed_pubkey_out: compressed_pubkey output
 * @param contract_out: contract output
 * @param path_with_curve: bip32 path and curve of the key
 * @return tz_exc: exception, SW_OK if none
 */
static inline tz_exc compute_pkh(cx_ecfp_public_key_t *const compressed_pubkey_out,
                                 parsed_contract_t *const contract_out,
                                 bip32_path_with_curve_t const *const path_with_curve) {
    tz_exc exc = SW_OK;
    cx_err_t error = CX_OK;

    TZ_ASSERT_NOT_NULL(path_with_curve);
    TZ_ASSERT_NOT_NULL(compressed_pubkey_out);
    TZ_ASSERT_NOT_NULL(contract_out);

    CX_CHECK(generate_public_key_hash(contract_out->hash,
                                      sizeof(contract_out->hash),
                                      compressed_pubkey_out,
                                      path_with_curve));

    contract_out->signature_type =
        derivation_type_to_signature_type(path_with_curve->derivation_type);
    TZ_ASSERT(contract_out->signature_type != SIGNATURE_TYPE_UNSET, EXC_MEMORY_ERROR);

    contract_out->originated = 0;

end:
    TZ_CONVERT_CX();
    return exc;
}

/**
 * @brief Parses implict contract
 *
 * @param out:  implict contract output
 * @param raw_signature_type: raw signature_type
 * @param hash: input hash
 * @return tz_parser_result: result of the parsing
 */
static tz_parser_result parse_implicit(
    parsed_contract_t *const out,
    raw_tezos_header_signature_type_t const *const raw_signature_type,
    uint8_t const hash[KEY_HASH_SIZE]) {
    tz_parser_result res = PARSER_CONTINUE;

    out->originated = 0;
    PARSER_CHECK(parse_raw_tezos_header_signature_type(raw_signature_type, &out->signature_type));
    memcpy(out->hash, hash, sizeof(out->hash));

    res = PARSER_DONE;

end:
    return res;
}

/**
 * @brief Helpers for sub parser
 *
 *       Subparsers: no function here should be called anywhere in
 *       this file without using the CALL_SUBPARSER macro above.
 *
 */
#define CALL_SUBPARSER_LN(func, line, ...) PARSER_CHECK(func(__VA_ARGS__, line))
#define CALL_SUBPARSER(func, ...)          CALL_SUBPARSER_LN(func, __LINE__, __VA_ARGS__)

#define NEXT_BYTE (byte)

/**
 * @brief Parses a Z number
 *
 * @param current_byte: the current read byte
 * @param state: parsing state
 * @param lineno: line number of the caller
 * @return tz_parser_result: result of the parsing
 */
static inline tz_parser_result parse_z(uint8_t current_byte,
                                       struct int_subparser_state *state,
                                       uint32_t lineno) {
    tz_parser_result res = PARSER_CONTINUE;

    if (state->lineno != lineno) {
        // New call; initialize.
        state->lineno = lineno;
        state->value = 0;
        state->shift = 0;
    }
    // Fails when the resulting shifted value overflows 64 bits
    if ((state->shift > 63u) || ((state->shift == 63u) && (current_byte != 1u))) {
        PARSER_FAIL();
    }
    state->value |= ((uint64_t) current_byte & 0x7Fu) << state->shift;
    state->shift += 7u;

    if ((current_byte & 0x80u) == 0u) {
        res = PARSER_DONE;
    }

end:
    return res;
}
#define PARSE_Z                                                             \
    ({                                                                      \
        CALL_SUBPARSER(parse_z, (byte), &(state)->subparser_state.integer); \
        (state)->subparser_state.integer.value;                             \
    })

/**
 * @brief Parses a wire type
 *
 * @param current_byte: the current read byte
 * @param state: parsing state
 * @param sizeof_type: size of the type
 * @param lineno: line number of the caller
 * @return tz_parser_result: result of the parsing
 */
static tz_parser_result parse_next_type(uint8_t current_byte,
                                        struct nexttype_subparser_state *state,
                                        uint32_t sizeof_type,
                                        uint32_t lineno) {
    tz_parser_result res = PARSER_CONTINUE;

#ifdef DEBUG
    if (sizeof_type > sizeof(state->body)) {
        PARSER_FAIL();  // Shouldn't happen, but error if it does and we're debugging. Neither side
                        // is dynamic.
    }
#endif

    if (state->lineno != lineno) {
        state->lineno = lineno;
        state->fill_idx = 0;
    }

    state->body.raw[state->fill_idx] = current_byte;
    state->fill_idx++;

    if (state->fill_idx == sizeof_type) {
        res = PARSER_DONE;
    }

    if (state->fill_idx > sizeof_type) {
        PARSER_FAIL();
    }

end:
    return res;
}
// do _NOT_ keep pointers to this data around.
#define NEXT_TYPE(type)                                                                          \
    ({                                                                                           \
        CALL_SUBPARSER(parse_next_type, byte, &(state->subparser_state.nexttype), sizeof(type)); \
        (const type *) &(state->subparser_state.nexttype.body);                                  \
    })

// End of subparsers.

/**
 * @brief Initialize the operation parser
 *
 * @param out: parsing output
 * @param path_with_curve: bip32 path and curve of the key
 * @param state: parsing state
 * @return tz_exc: exception, SW_OK if none
 */
static tz_exc parse_operations_init(struct parsed_operation_group *const out,
                                    bip32_path_with_curve_t const *const path_with_curve,
                                    struct parse_state *const state) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(out);
    TZ_ASSERT_NOT_NULL(path_with_curve);

    memset(out, 0, sizeof(*out));

    out->operation.tag = OPERATION_TAG_NONE;

    TZ_CHECK(compute_pkh(&out->public_key, &out->signing, path_with_curve));

    // Start out with source = signing, for reveals
    memcpy(&out->operation.source, &out->signing, sizeof(out->signing));

    state->op_step = 0;
    state->subparser_state.integer.lineno = -1;
    state->tag = OPERATION_TAG_NONE;  // This and the rest shouldn't be required.

end:
    return exc;
}

/// Named steps in the top-level state machine
#define STEP_END_OF_MESSAGE       -1
#define STEP_OP_TYPE_DISPATCH     10001
#define STEP_AFTER_MANAGER_FIELDS 10002
#define STEP_HAS_DELEGATE         10003

bool parse_operations_final(struct parse_state *const state,
                            struct parsed_operation_group *const out) {
    if ((state == NULL) || (out == NULL)) {
        return false;
    }
    if ((out->operation.tag == OPERATION_TAG_NONE) && !out->has_reveal) {
        return false;
    }
    return ((state->op_step == STEP_END_OF_MESSAGE) || (state->op_step == 1));
}

/**
 * @brief Parse one bytes regarding the current parsing state
 *
 * @param byte: byte to read
 * @param state: parsing state
 * @param out: parsing output
 * @return tz_parser_result: result of the parsing
 */
static inline tz_parser_result parse_byte(uint8_t byte,
                                          struct parse_state *const state,
                                          struct parsed_operation_group *const out) {
    tz_parser_result res = PARSER_CONTINUE;

// OP_STEP finishes the current state transition, setting the state, and introduces the next state.
// For linear chains of states, this keeps the code structurally similar to equivalent imperative
// parsing code.
#define OP_STEP                \
    state->op_step = __LINE__; \
    break;                     \
    case __LINE__:

// The same as OP_STEP, but with a particular name, such that we could jump to this state.
#define OP_NAMED_STEP(name) \
    state->op_step = name;  \
    break;                  \
    case name:

// "jump" to specific state: (set state to foo and return.)
#define JMP(step)          \
    state->op_step = step; \
    break;

// Set the next state to start-of-payload; used after reveal.
#define JMP_TO_TOP JMP(1)

// Conditionally set the next state.
#define OP_JMPIF(step, cond)   \
    if (cond) {                \
        state->op_step = step; \
        break;                 \
    }

    switch (state->op_step) {
        case STEP_HARD_FAIL:
            PARSER_FAIL();

        case STEP_END_OF_MESSAGE:
            PARSER_FAIL();  // We already hit a hard end of message; fail.

        case 0: {
            // Ignore block hash
            NEXT_TYPE(struct operation_group_header);
        }

            OP_NAMED_STEP(1)

            state->tag = NEXT_BYTE;

            OP_STEP

            // Parse 'source'
            switch (state->tag) {
                // Tags that don't have "originated" byte only support tz accounts, not KT or tz.
                case OPERATION_TAG_DELEGATION:
                case OPERATION_TAG_REVEAL: {
                    struct implicit_contract const *const implicit_source =
                        NEXT_TYPE(struct implicit_contract);

                    out->operation.source.originated = 0;
                    PARSER_CHECK(parse_raw_tezos_header_signature_type(
                        &implicit_source->signature_type,
                        &out->operation.source.signature_type));

                    memcpy(out->operation.source.hash,
                           implicit_source->pkh,
                           sizeof(out->operation.source.hash));
                    break;
                }

                default:
                    PARSER_FAIL();
                    break;
            }

            // If the source is an implicit contract,...
            if (!out->operation.source.originated) {
                // ... it had better match our key, otherwise why are we signing it?
                PARSER_ASSERT(COMPARE(out->operation.source, out->signing) == 0);
            }
            // OK, it passes muster.

            OP_STEP

            // out->operation.source IS NORMALIZED AT THIS POINT

            // Parse common fields for non-governance related operations.

            out->total_fee += PARSE_Z;  // fee
            OP_STEP
            PARSE_Z;  // counter
            OP_STEP
            PARSE_Z;  // gas limit
            OP_STEP
            out->total_storage_limit += PARSE_Z;  // storage limit

            OP_JMPIF(STEP_AFTER_MANAGER_FIELDS, state->tag != OPERATION_TAG_REVEAL)

            OP_STEP

            // We know this is a reveal

            // Public key up next! Ensure it matches signing key.
            {
                raw_tezos_header_signature_type_t const *const sig_type =
                    NEXT_TYPE(raw_tezos_header_signature_type_t);
                signature_type_t reveal_signature_type = {0};
                PARSER_CHECK(
                    parse_raw_tezos_header_signature_type(sig_type, &reveal_signature_type));
                PARSER_ASSERT(reveal_signature_type == out->signing.signature_type);
            }

            OP_STEP

            {
                size_t klen = out->public_key.W_len;

                // klen must match one of the field sizes in the public_key union

                CALL_SUBPARSER(parse_next_type, byte, &(state->subparser_state.nexttype), klen);

                PARSER_ASSERT(memcmp(out->public_key.W,
                                     &(state->subparser_state.nexttype.body.raw),
                                     klen) == 0);

                out->has_reveal = true;

                JMP_TO_TOP;
            }

        case STEP_AFTER_MANAGER_FIELDS:  // Anything but a reveal

            // We are only currently allowing one non-reveal operation
            PARSER_ASSERT(out->operation.tag == OPERATION_TAG_NONE);

            // This is the one allowable non-reveal operation per set

            out->operation.tag = state->tag;

            // Deliberate epsilon-transition.
            state->op_step = STEP_OP_TYPE_DISPATCH;
            __attribute__((fallthrough));
        default:

            if (state->tag == OPERATION_TAG_DELEGATION) {
                switch (state->op_step) {
                    case STEP_OP_TYPE_DISPATCH: {
                        bool delegate_present = NEXT_BYTE != 0u;

                        OP_JMPIF(STEP_HAS_DELEGATE, delegate_present)
                    }
                        // Else branch: Encode "not present"
                        out->operation.destination.originated = 0;
                        out->operation.destination.signature_type = SIGNATURE_TYPE_UNSET;

                        JMP_TO_TOP;  // These go back to the top to catch any reveals.

                    case STEP_HAS_DELEGATE: {
                        const struct delegation_contents *dlg =
                            NEXT_TYPE(struct delegation_contents);
                        PARSER_CHECK(parse_implicit(&out->operation.destination,
                                                    &dlg->signature_type,
                                                    dlg->hash));
                    }
                        JMP_TO_TOP;  // These go back to the top to catch any reveals.
                    default:         // Any other tag; probably not possible here.
                        PARSER_FAIL();
                }
            }
    }

end:
    if (res == PARSER_ERROR) {
        global.apdu.u.sign.parse_state.op_step = STEP_HARD_FAIL;
    }
    return res;
}

#define G global.apdu.u.sign

tz_exc parse_operations(buffer_t *buf,
                        struct parsed_operation_group *const out,
                        bip32_path_with_curve_t const *const path_with_curve) {
    tz_exc exc = SW_OK;
    uint8_t byte;

    TZ_CHECK(parse_operations_init(out, path_with_curve, &G.parse_state));

    while (buffer_read_u8(buf, &byte) == true) {
        TZ_ASSERT(parse_byte(byte, &G.parse_state, out) != PARSER_ERROR, EXC_PARSE_ERROR);
        PRINTF("Byte: %x - Next op_step state: %d\n", byte, G.parse_state.op_step);
    }

end:
    return exc;
}
