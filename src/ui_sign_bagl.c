#ifdef HAVE_BAGL
#include "apdu_sign.h"

#include "apdu.h"
#include "baking_auth.h"
#include "base58.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "swap/swap_lib_calls.h"

#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

#define B2B_BLOCKBYTES 128


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

#ifdef BAKING_APP  // ----------------------------------------------------------

__attribute__((noreturn)) void prompt_register_delegate(ui_callback_t const ok_cb,
                                                        ui_callback_t const cxl_cb) {
    if (!G.maybe_ops.is_valid) THROW(EXC_MEMORY_ERROR);

    init_screen_stack();
    push_ui_callback("Register", copy_string, "as delegate?");
    push_ui_callback("Address", bip32_path_with_curve_to_pkh_string, &global.path_with_curve);
    push_ui_callback("Fee", microtez_to_string_indirect, &G.maybe_ops.v.total_fee);

    ux_confirm_screen(ok_cb, cxl_cb);
}

#else  // ifdef BAKING_APP -----------------------------------------------------

static bool sign_unsafe_ok(void) {
    delayed_send(perform_signature(false, false));
    return true;
}

#define MAX_NUMBER_CHARS (MAX_INT_DIGITS + 2)  // include decimal point and terminating null

bool prompt_transaction(struct parsed_operation_group const *const ops,
                        bip32_path_with_curve_t const *const key,
                        ui_callback_t ok,
                        ui_callback_t cxl) {
    check_null(ops);
    check_null(key);

    if (called_from_swap) {
        if (is_safe_to_swap() == true) {
            // We're called from swap and we've verified that the data is correct. Sign it.
            ok();
            // Clear all data.
            clear_data();
            // Exit properly.
            os_sched_exit(0);
        } else {
            // Send the error message back in response.
            cxl();
            // Exit with error code.
            os_sched_exit(1);
        }
    }

    switch (ops->operation.tag) {
        default:
            PARSE_ERROR();

        case OPERATION_TAG_PROPOSAL: {
            init_screen_stack();
            push_ui_callback("Confirm", copy_string, "Proposal");
            push_ui_callback("Source", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Period",
                             number_to_string_indirect32,
                             &ops->operation.proposal.voting_period);
            push_ui_callback("Protocol",
                             protocol_hash_to_string,
                             ops->operation.proposal.protocol_hash);

            ux_confirm_screen(ok, cxl);
        }

        case OPERATION_TAG_BALLOT: {
            char *vote;

            switch (ops->operation.ballot.vote) {
                case BALLOT_VOTE_YEA:
                    vote = "Yea";
                    break;
                case BALLOT_VOTE_NAY:
                    vote = "Nay";
                    break;
                case BALLOT_VOTE_PASS:
                    vote = "Pass";
                    break;
            }

            init_screen_stack();
            push_ui_callback("Confirm Vote", copy_string, vote);
            push_ui_callback("Source", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Protocol",
                             protocol_hash_to_string,
                             ops->operation.ballot.protocol_hash);
            push_ui_callback("Period",
                             number_to_string_indirect32,
                             &ops->operation.ballot.voting_period);

            ux_confirm_screen(ok, cxl);
        }

        case OPERATION_TAG_ATHENS_ORIGINATION:
        case OPERATION_TAG_BABYLON_ORIGINATION: {
            if (!(ops->operation.flags & ORIGINATION_FLAG_SPENDABLE)) return false;

            init_screen_stack();
            push_ui_callback("Confirm", copy_string, "Origination");
            push_ui_callback("Amount", microtez_to_string_indirect, &ops->operation.amount);
            push_ui_callback("Fee", microtez_to_string_indirect, &ops->total_fee);
            push_ui_callback("Source", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Manager", parsed_contract_to_string, &ops->operation.destination);

            bool const delegatable = ops->operation.flags & ORIGINATION_FLAG_DELEGATABLE;
            bool const has_delegate =
                ops->operation.delegate.signature_type != SIGNATURE_TYPE_UNSET;
            if (delegatable && has_delegate) {
                push_ui_callback("Delegate", parsed_contract_to_string, &ops->operation.delegate);
            } else if (delegatable && !has_delegate) {
                push_ui_callback("Delegate", copy_string, "Any");
            } else if (!delegatable && has_delegate) {
                push_ui_callback("Fixed Delegate",
                                 parsed_contract_to_string,
                                 &ops->operation.delegate);
            } else if (!delegatable && !has_delegate) {
                push_ui_callback("Delegation", copy_string, "Disabled");
            }
            push_ui_callback("Storage Limit",
                             number_to_string_indirect64,
                             &ops->total_storage_limit);

            ux_confirm_screen(ok, cxl);
        }
        case OPERATION_TAG_ATHENS_DELEGATION:
        case OPERATION_TAG_BABYLON_DELEGATION: {
            bool const withdrawal =
                ops->operation.destination.originated == 0 &&
                ops->operation.destination.signature_type == SIGNATURE_TYPE_UNSET;

            char *type_msg;
            if (withdrawal) {
                type_msg = "Withdraw";
            } else {
                type_msg = "Confirm";
            }
            init_screen_stack();
            push_ui_callback(type_msg, copy_string, "Delegation");

            push_ui_callback("Fee", microtez_to_string_indirect, &ops->total_fee);
            push_ui_callback("Source", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Delegate", parsed_contract_to_string, &ops->operation.destination);
            push_ui_callback("Delegate Name",
                             lookup_parsed_contract_name,
                             &ops->operation.destination);
            push_ui_callback("Storage Limit",
                             number_to_string_indirect64,
                             &ops->total_storage_limit);

            ux_confirm_screen(ok, cxl);
        }

        case OPERATION_TAG_ATHENS_TRANSACTION:
        case OPERATION_TAG_BABYLON_TRANSACTION: {
            init_screen_stack();
            push_ui_callback("Confirm", copy_string, "Transaction");
            push_ui_callback("Amount", microtez_to_string_indirect, &ops->operation.amount);
            push_ui_callback("Fee", microtez_to_string_indirect, &ops->total_fee);
            push_ui_callback("Source", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Destination", parsed_contract_to_string, &ops->operation.destination);
            push_ui_callback("Storage Limit",
                             number_to_string_indirect64,
                             &ops->total_storage_limit);

            ux_confirm_screen(ok, cxl);
        }
        case OPERATION_TAG_NONE: {
            init_screen_stack();
            push_ui_callback("Reveal Key", copy_string, "To Blockchain");
            push_ui_callback("Key", parsed_contract_to_string, &ops->operation.source);
            push_ui_callback("Fee", microtez_to_string_indirect, &ops->total_fee);
            push_ui_callback("Storage Limit",
                             number_to_string_indirect64,
                             &ops->total_storage_limit);

            ux_confirm_screen(ok, cxl);
        }
    }
}

size_t wallet_sign_complete(uint8_t instruction, uint8_t magic_byte, volatile uint32_t* flags) {
    (void) flags;
    char *ops;
    if (magic_byte == MAGIC_BYTE_UNSAFE_OP3) {
        ops = "Michelson";
    } else {
        ops = "Operation";
    }

    if (instruction == INS_SIGN_UNSAFE) {
        G.message_data_as_buffer.bytes = (uint8_t *) &G.message_data;
        G.message_data_as_buffer.size = sizeof(G.message_data);
        G.message_data_as_buffer.length = G.message_data_length;
        init_screen_stack();
        push_ui_callback("Pre-hashed", copy_string, ops);
        // Base58 encoding of 32-byte hash is 43 bytes long.
        push_ui_callback("Sign Hash", buffer_to_base58, &G.message_data_as_buffer);
        ux_confirm_screen(sign_unsafe_ok, sign_reject);
    } else {
        ui_callback_t const ok_c =
            instruction == INS_SIGN_WITH_HASH ? sign_with_hash_ok : sign_without_hash_ok;

        switch (G.magic_byte) {
            case MAGIC_BYTE_BLOCK:
            case MAGIC_BYTE_BAKING_OP:
            default:
                PARSE_ERROR();
            case MAGIC_BYTE_UNSAFE_OP:
                if (!G.maybe_ops.is_valid || !prompt_transaction(&G.maybe_ops.v,
                                                                 &global.path_with_curve,
                                                                 ok_c,
                                                                 sign_reject)) {
                    goto unsafe;
                }

            case MAGIC_BYTE_UNSAFE_OP2:
            case MAGIC_BYTE_UNSAFE_OP3:
                goto unsafe;
        }
    unsafe:
        G.message_data_as_buffer.bytes = (uint8_t *) &G.final_hash;
        G.message_data_as_buffer.size = sizeof(G.final_hash);
        G.message_data_as_buffer.length = sizeof(G.final_hash);
        init_screen_stack();
        // Base58 encoding of 32-byte hash is 43 bytes long.
        push_ui_callback("Unrecognized", copy_string, ops);
        push_ui_callback("Sign Hash", buffer_to_base58, &G.message_data_as_buffer);
        ux_confirm_screen(ok_c, sign_reject);
    }
}

#endif  // ifdef BAKING_APP ----------------------------------------------------
#endif // HAVE_BAGL
