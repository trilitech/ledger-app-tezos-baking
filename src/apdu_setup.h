#pragma once

#ifdef BAKING_APP

#include "apdu.h"

#include <stddef.h>
#include <stdint.h>

size_t handle_apdu_setup(uint8_t instruction);

struct setup_wire {
    uint32_t main_chain_id;
    struct {
        uint32_t main;
        uint32_t test;
    } hwm;
    struct bip32_path_wire bip32_path;
} __attribute__((packed));

__attribute__((noreturn)) void prompt_setup(ui_callback_t const ok_cb,
                                            ui_callback_t const cxl_cb);
#endif  // #ifdef BAKING_APP
