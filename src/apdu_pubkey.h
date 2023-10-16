#pragma once

#include "apdu.h"

size_t handle_apdu_get_public_key(uint8_t instruction, volatile uint32_t* flags);

void prompt_address(
    bool baking,
    ui_callback_t ok_cb,
    ui_callback_t cxl_cb);
