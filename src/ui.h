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

void ui_initial_screen(void);
void ui_init(void);
void ui_refresh(void);

void exit_app(void);  // Might want to send it arguments to use as callback

void ux_confirm_screen(ui_callback_t ok_c, ui_callback_t cxl_c);

void ux_idle_screen(ui_callback_t ok_c, ui_callback_t cxl_c);

void ux_empty_screen(void);
/* Initializes the formatter stack. Should be called once before calling `push_ui_callback()`. */
void init_screen_stack();
/* User MUST call `init_screen_stack()` before calling this function for the first time. */
void push_ui_callback(char *title, string_generation_callback cb, void *data);
