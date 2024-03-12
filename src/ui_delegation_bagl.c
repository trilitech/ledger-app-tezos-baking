/* Tezos Ledger application - Delegate BAGL UI handling

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
#include "apdu_sign.h"
#include "ui_delegation.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

#define B2B_BLOCKBYTES 128

__attribute__((noreturn)) void prompt_delegation(ui_callback_t const ok_cb,
                                                 ui_callback_t const cxl_cb) {
    if (!G.maybe_ops.is_valid) {
        THROW(EXC_MEMORY_ERROR);
    }

    init_screen_stack();
    push_ui_callback("Register", copy_string, "as delegate?");
    push_ui_callback("Address", bip32_path_with_curve_to_pkh_string, &global.path_with_curve);
    push_ui_callback("Fee", microtez_to_string_indirect, &G.maybe_ops.v.total_fee);

    ux_confirm_screen(ok_cb, cxl_cb);
    __builtin_unreachable();
}

#endif  // HAVE_BAGL
