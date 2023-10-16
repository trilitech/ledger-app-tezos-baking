#pragma once

#include "apdu.h"

size_t handle_apdu_sign(uint8_t instruction, volatile uint32_t* flags);
size_t handle_apdu_sign_with_hash(uint8_t instruction, volatile uint32_t* flags);

#ifdef BAKING_APP  // ----------------------------------------------------------

void prompt_register_delegate(ui_callback_t const ok_cb, ui_callback_t const cxl_cb);

#endif  // ifdef BAKING_APP ----------------------------------------------------

int perform_signature(bool const on_hash, bool const send_hash);
