#pragma once

#ifdef BAKING_APP

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t handle_apdu_reset(uint8_t instruction, volatile uint32_t *flags);
size_t handle_apdu_query_auth_key(uint8_t instruction,
                                  volatile uint32_t *flags);
size_t handle_apdu_query_auth_key_with_curve(uint8_t instruction,
                                             volatile uint32_t *flags);
size_t handle_apdu_main_hwm(uint8_t instruction, volatile uint32_t *flags);
size_t handle_apdu_all_hwm(uint8_t instruction, volatile uint32_t *flags);
size_t handle_apdu_deauthorize(uint8_t instruction, volatile uint32_t *flags);
void ui_baking_reset(volatile uint32_t *flags);
bool reset_ok(void);

#endif // #ifdef BAKING_APP
