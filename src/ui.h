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
void exit_app(void);

#ifdef HAVE_BAGL

/**
 * @brief Updates the baking screens
 *
 */
void update_baking_idle_screens(void);

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

#endif  // HAVE_BAGL
