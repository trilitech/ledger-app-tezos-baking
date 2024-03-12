/* Tezos Ledger application - Reset BAGL UI handling

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

#ifdef HAVE_BAGL

#include "ui_reset.h"

#include "apdu_reset.h"
#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.baking

__attribute__((noreturn)) void prompt_reset(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    init_screen_stack();
    push_ui_callback("Reset HWM", number_to_string_indirect32, &G.reset_level);

    ux_confirm_screen(ok_cb, cxl_cb);
    __builtin_unreachable();
}

#endif  // HAVE_BAGL
