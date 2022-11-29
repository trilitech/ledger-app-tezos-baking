#pragma once

#include "apdu.h"

size_t handle_apdu_sign(uint8_t instruction);
size_t handle_apdu_sign_with_hash(uint8_t instruction);

#ifdef BAKING_APP  // ----------------------------------------------------------

__attribute__((noreturn)) void prompt_register_delegate(ui_callback_t const ok_cb,
                                                        ui_callback_t const cxl_cb);

#else  // ifdef BAKING_APP -----------------------------------------------------
bool prompt_transaction(struct parsed_operation_group const *const ops,
                        bip32_path_with_curve_t const *const key,
                        ui_callback_t ok,
                        ui_callback_t cxl);

size_t wallet_sign_complete(uint8_t instruction, uint8_t magic_byte);
#endif  // ifdef BAKING_APP ----------------------------------------------------

int perform_signature(bool const on_hash, bool const send_hash);
