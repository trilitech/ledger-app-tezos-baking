#pragma once

#ifdef BAKING_APP

#include "apdu.h"

#include <stddef.h>
#include <stdint.h>

size_t handle_apdu_setup(uint8_t instruction, volatile uint32_t* flags);

void prompt_setup(ui_callback_t const ok_cb,
                  ui_callback_t const cxl_cb);
#endif  // #ifdef BAKING_APP
