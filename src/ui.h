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
 * @brief Initializes the formatter stack
 *
 *        Must be called once before calling `push_ui_callback()` for the first time
 *
 */
void init_screen_stack(void);

/**
 * @brief Pushes a new screen to the flow
 *
 *        Must call `init_screen_stack()` before calling this function for the first time
 *
 * @param title: title of the screen
 * @param cb: callback to generate the string version of the data
 * @param data: data to display on the screen
 */
void push_ui_callback(const char *title, string_generation_callback cb, const void *data);

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
 * @brief Prepare confirmation screens
 *
 * @param ok_c: accept callback
 * @param cxl_c: cancel callback
 */
void ux_confirm_screen(ui_callback_t ok_c, ui_callback_t cxl_c);

#endif  // HAVE_BAGL
