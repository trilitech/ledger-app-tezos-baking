/* Tezos Ledger application - Common UI functions

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2019 Obsidian Systems

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

#pragma once

#include "os_io_seproxyhal.h"
#include "ux.h"

#include "types.h"

#include "keys.h"

/**
 * @brief Starts the idle flow
 *
 */
void ui_initial_screen(void);

/**
 * @brief Exits the app
 *
 */
void __attribute__((noreturn)) app_exit(void);

#ifdef HAVE_BAGL

/**
 * @brief Calculates the chain id for the idle screens
 *
 * @return tz_exc: exception, SW_OK if none
 */
tz_exc calculate_idle_screen_chain_id(void);

/**
 * @brief Calculates the authorized key for the idle screens
 *
 * @return tz_exc: exception, SW_OK if none
 */
tz_exc calculate_idle_screen_authorized_key(void);

/**
 * @brief Calculates the HWM for the idle screens
 *
 * @return tz_exc: exception, SW_OK if none
 */
tz_exc calculate_idle_screen_hwm(void);

/**
 * @brief Calculates baking values for the idle screens
 *
 * @return tz_exc: exception, SW_OK if none
 */
tz_exc calculate_baking_idle_screens_data(void);

/**
 * @brief Prepare confirmation screens callbacks
 *
 * @param ok_c: accept callback
 * @param cxl_c: cancel callback
 */
void ux_prepare_confirm_callbacks(ui_callback_t ok_c, ui_callback_t cxl_c);

/**
 * @brief Confirmation flow
 *
 *        - Initial screen
 *        - Values
 *        - Reject screen
 *        - Accept screen
 *
 */
extern const ux_flow_step_t ux_eye_step;
extern const ux_flow_step_t ux_prompt_flow_reject_step;
extern const ux_flow_step_t ux_prompt_flow_accept_step;
#define UX_CONFIRM_FLOW(flow_name, ...)  \
    UX_FLOW(flow_name,                   \
            &ux_eye_step,                \
            __VA_ARGS__,                 \
            &ux_prompt_flow_reject_step, \
            &ux_prompt_flow_accept_step)

/**
 * @brief Refreshes all screens of the current flow
 *
 */
void refresh_screens(void);

#endif  // HAVE_BAGL
