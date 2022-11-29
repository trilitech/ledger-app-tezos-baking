#pragma once

#include "apdu.h"

size_t handle_apdu_get_public_key(uint8_t instruction);

__attribute__((noreturn)) void prompt_address(
#ifndef BAKING_APP
    __attribute__((unused))
#endif
    bool baking,
    ui_callback_t ok_cb,
    ui_callback_t cxl_cb);
