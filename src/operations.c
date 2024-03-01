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

#define STEP_HARD_FAIL -2

/**
 * @brief Raises an exception and force hard fail if continue to parse
 *
 */
__attribute__((noreturn)) static void parse_error(void) {
    global.apdu.u.sign.parse_state.op_step = STEP_HARD_FAIL;
    THROW(EXC_PARSE_ERROR);
}

#define PARSE_ERROR() parse_error()

/// Conversion/check functions

/**
 * @brief Get signature_type from a raw signature_type
 *
 * @param raw_signature_type: raw signature_type
 * @return signature_type_t: signature_type result
 */
static inline signature_type_t parse_raw_tezos_header_signature_type(
    raw_tezos_header_signature_type_t const *const raw_signature_type) {
    check_null(raw_signature_type);
    switch (READ_UNALIGNED_BIG_ENDIAN(uint8_t, &raw_signature_type->v)) {
        case 0:
            return SIGNATURE_TYPE_ED25519;
        case 1:
            return SIGNATURE_TYPE_SECP256K1;
        case 2:
            return SIGNATURE_TYPE_SECP256R1;
        default:
            PARSE_ERROR();
    }
}

/**
 * @brief Extracts a compressed_pubkey and a contract from a key
 *
 * @param compressed_pubkey_out: compressed_pubkey output
 * @param contract_out: contract output
 * @param derivation_type: curve of the key
 * @param bip32_path: bip32 path of the key
 */
static inline void compute_pkh(cx_ecfp_public_key_t *const compressed_pubkey_out,
                               parsed_contract_t *const contract_out,
                               derivation_type_t const derivation_type,
                               bip32_path_t const *const bip32_path) {
    check_null(bip32_path);
    check_null(compressed_pubkey_out);
    check_null(contract_out);
    cx_ecfp_public_key_t pubkey = {0};
    generate_public_key(&pubkey, derivation_type, bip32_path);
    public_key_hash(contract_out->hash,
                    sizeof(contract_out->hash),
                    compressed_pubkey_out,
                    derivation_type,
                    &pubkey);
    contract_out->signature_type = derivation_type_to_signature_type(derivation_type);
    if (contract_out->signature_type == SIGNATURE_TYPE_UNSET) {
        THROW(EXC_MEMORY_ERROR);
    }
    contract_out->originated = 0;
}

/**
 * @brief Parses implict contract
 *
 * @param out:  implict contract output
 * @param raw_signature_type: raw signature_type
 * @param hash: input hash
 */
static inline void parse_implicit(parsed_contract_t *const out,
                                  raw_tezos_header_signature_type_t const *const raw_signature_type,
                                  uint8_t const hash[HASH_SIZE]) {
    check_null(raw_signature_type);
    out->originated = 0;
    out->signature_type = parse_raw_tezos_header_signature_type(raw_signature_type);
    memcpy(out->hash, hash, sizeof(out->hash));
}

/**
 * @brief Helpers for sub parser
 *
 *       Subparsers: no function here should be called anywhere in
 *       this file without using the CALL_SUBPARSER macro above.
 *
 */
#define CALL_SUBPARSER_LN(func, line, ...) \
    if (func(__VA_ARGS__, line)) {         \
        return true;                       \
    }
#define CALL_SUBPARSER(func, ...) CALL_SUBPARSER_LN(func, __LINE__, __VA_ARGS__)

#define NEXT_BYTE (byte)

// TODO: this function cannot parse z values than would not fit in a uint64
/**
 * @brief Parses a Z number
 *
 * @param current_byte: the current read byte
 * @param state: parsing state
 * @param lineno: line number of the caller
 * @return bool: if has finished to read the number
 */
static inline bool parse_z(uint8_t current_byte,
                           struct int_subparser_state *state,
                           uint32_t lineno) {
    if (state->lineno != lineno) {
        // New call; initialize.
        state->lineno = lineno;
        state->value = 0;
        state->shift = 0;
    }
    // Fails when the resulting shifted value overflows 64 bits
    if (state->shift > 63u || (state->shift == 63u && current_byte != 1u)) {
        PARSE_ERROR();
    }
    state->value |= ((uint64_t) current_byte & 0x7Fu) << state->shift;
    state->shift += 7u;
    return current_byte & 0x80u;  // Return true if we need more bytes.
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
 * @return bool: if has finished to read the type
 */
static inline bool parse_next_type(uint8_t current_byte,
                                   struct nexttype_subparser_state *state,
                                   uint32_t sizeof_type,
                                   uint32_t lineno) {
#ifdef DEBUG
    if (sizeof_type > sizeof(state->body)) {
        PARSE_ERROR();  // Shouldn't happen, but error if it does and we're debugging. Neither side
                        // is dynamic.
    }
#endif

    if (state->lineno != lineno) {
        state->lineno = lineno;
        state->fill_idx = 0;
    }

    state->body.raw[state->fill_idx] = current_byte;
    state->fill_idx++;

    return state->fill_idx < sizeof_type;  // Return true if we need more bytes.
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
 * @param derivation_type: curve of the key
 * @param bip32_path: bip32 path of the key
 * @param state: parsing state
 */
static void parse_operations_init(struct parsed_operation_group *const out,
                                  derivation_type_t derivation_type,
                                  bip32_path_t const *const bip32_path,
                                  struct parse_state *const state) {
    check_null(out);
    check_null(bip32_path);
    memset(out, 0, sizeof(*out));

    out->operation.tag = OPERATION_TAG_NONE;

    compute_pkh(&out->public_key, &out->signing, derivation_type, bip32_path);

    // Start out with source = signing, for reveals
    // TODO: This is slightly hackish
    memcpy(&out->operation.source, &out->signing, sizeof(out->signing));

    state->op_step = 0;
    state->subparser_state.integer.lineno = -1;
    state->tag = OPERATION_TAG_NONE;  // This and the rest shouldn't be required.
}

/// Named steps in the top-level state machine
#define STEP_END_OF_MESSAGE       -1
#define STEP_OP_TYPE_DISPATCH     10001
#define STEP_AFTER_MANAGER_FIELDS 10002
#define STEP_HAS_DELEGATE         10003

bool parse_operations_final(struct parse_state *const state,
                            struct parsed_operation_group *const out) {
    if (out->operation.tag == OPERATION_TAG_NONE && !out->has_reveal) {
        return false;
    }
    return state->op_step == STEP_END_OF_MESSAGE || state->op_step == 1;
}

/**
 * @brief Parse one bytes regarding the current parsing state
 *
 * @param byte: byte to read
 * @param state: parsing state
 * @param out: parsing output
 * @return bool: returns true on success
 */
static inline bool parse_byte(uint8_t byte,
                              struct parse_state *const state,
                              struct parsed_operation_group *const out) {
// OP_STEP finishes the current state transition, setting the state, and introduces the next state.
// For linear chains of states, this keeps the code structurally similar to equivalent imperative
// parsing code.
#define OP_STEP                \
    state->op_step = __LINE__; \
    return true;               \
    case __LINE__:

// The same as OP_STEP, but with a particular name, such that we could jump to this state.
#define OP_NAMED_STEP(name) \
    state->op_step = name;  \
    return true;            \
    case name:

// "jump" to specific state: (set state to foo and return.)
#define JMP(step)          \
    state->op_step = step; \
    return true

// Set the next state to start-of-payload; used after reveal.
#define JMP_TO_TOP JMP(1)

// Conditionally set the next state.
#define OP_JMPIF(step, cond)   \
    if (cond) {                \
        state->op_step = step; \
        return true;           \
    }

    switch (state->op_step) {
        case STEP_HARD_FAIL:
            PARSE_ERROR();

        case STEP_END_OF_MESSAGE:
            PARSE_ERROR();  // We already hit a hard end of message; fail.

        case 0: {
            // Verify magic byte, ignore block hash
            const struct operation_group_header *ogh = NEXT_TYPE(struct operation_group_header);
            if (ogh->magic_byte != MAGIC_BYTE_UNSAFE_OP) {
                PARSE_ERROR();
            }
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
                    out->operation.source.signature_type =
                        parse_raw_tezos_header_signature_type(&implicit_source->signature_type);
                    memcpy(out->operation.source.hash,
                           implicit_source->pkh,
                           sizeof(out->operation.source.hash));
                    break;
                }

                default:
                    PARSE_ERROR();
            }

            // If the source is an implicit contract,...
            if (!out->operation.source.originated) {
                // ... it had better match our key, otherwise why are we signing it?
                if (COMPARE(&out->operation.source, &out->signing) != 0) {
                    PARSE_ERROR();
                }
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
                if (parse_raw_tezos_header_signature_type(sig_type) !=
                    out->signing.signature_type) {
                    PARSE_ERROR();
                }
            }

            OP_STEP

            {
                size_t klen = out->public_key.W_len;

                CALL_SUBPARSER(parse_next_type, byte, &(state->subparser_state.nexttype), klen);

                if (memcmp(out->public_key.W, &(state->subparser_state.nexttype.body.raw), klen) !=
                    0) {
                    PARSE_ERROR();
                }

                out->has_reveal = true;

                JMP_TO_TOP;
            }

        case STEP_AFTER_MANAGER_FIELDS:  // Anything but a reveal

            if (out->operation.tag != OPERATION_TAG_NONE) {
                // We are only currently allowing one non-reveal operation
                PARSE_ERROR();
            }

            // This is the one allowable non-reveal operation per set

            out->operation.tag = state->tag;

            // Deliberate epsilon-transition.
            state->op_step = STEP_OP_TYPE_DISPATCH;
            __attribute__((fallthrough));
        default:

            switch (state->tag) {
                case OPERATION_TAG_DELEGATION:
                    switch (state->op_step) {
                        case STEP_OP_TYPE_DISPATCH: {
                            uint8_t delegate_present = NEXT_BYTE;

                            OP_JMPIF(STEP_HAS_DELEGATE, delegate_present)
                        }
                            // Else branch: Encode "not present"
                            out->operation.destination.originated = 0;
                            out->operation.destination.signature_type = SIGNATURE_TYPE_UNSET;

                            JMP_TO_TOP;  // These go back to the top to catch any reveals.

                        case STEP_HAS_DELEGATE: {
                            const struct delegation_contents *dlg =
                                NEXT_TYPE(struct delegation_contents);
                            parse_implicit(&out->operation.destination,
                                           &dlg->signature_type,
                                           dlg->hash);
                        }
                            JMP_TO_TOP;  // These go back to the top to catch any reveals.
                    }

                default:  // Any other tag; probably not possible here.
                    PARSE_ERROR();
            }
    }

    PARSE_ERROR();  // Probably not reachable, but removes a warning.
}

#define G global.apdu.u.sign

/**
 * @brief Parses a group of operation
 *
 *        Throws on parsing failure
 *
 * @param out: output
 * @param data: input
 * @param length: input length
 * @param derivation_type: curve of the key
 * @param bip32_path: bip32 path of the key
 */
static void parse_operations_throws_parse_error(struct parsed_operation_group *const out,
                                                void const *const data,
                                                size_t length,
                                                derivation_type_t derivation_type,
                                                bip32_path_t const *const bip32_path) {
    size_t ix = 0;

    parse_operations_init(out, derivation_type, bip32_path, &G.parse_state);

    while (ix < length) {
        uint8_t byte = ((uint8_t *) data)[ix];
        parse_byte(byte, &G.parse_state, out);
        PRINTF("Byte: %x - Next op_step state: %d\n", byte, G.parse_state.op_step);
        ix++;
    }

    if (!parse_operations_final(&G.parse_state, out)) {
        PARSE_ERROR();
    }
}

bool parse_operations(struct parsed_operation_group *const out,
                      uint8_t const *const data,
                      size_t length,
                      derivation_type_t derivation_type,
                      bip32_path_t const *const bip32_path) {
    BEGIN_TRY {
        TRY {
            parse_operations_throws_parse_error(out, data, length, derivation_type, bip32_path);
        }
        CATCH(EXC_PARSE_ERROR) {
            return false;
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    return true;
}
