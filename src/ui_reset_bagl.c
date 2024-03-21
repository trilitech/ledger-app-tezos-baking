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

/**
 * @brief This structure represents a context needed for reset screens navigation
 *
 */
typedef struct {
    char reset_level[MAX_INT_DIGITS + 1u];
} ResetContext_t;

/// Current reset context
static ResetContext_t reset_context;

UX_STEP_NOCB(ux_reset_level_step, bnnn_paging, {"Reset HWM", reset_context.reset_level});

UX_CONFIRM_FLOW(ux_reset_flow, &ux_reset_level_step);

void prompt_reset(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    memset(&reset_context, 0, sizeof(reset_context));

    number_to_string_indirect32(reset_context.reset_level,
                                sizeof(reset_context.reset_level),
                                &G.reset_level);

    ux_prepare_confirm_callbacks(ok_cb, cxl_cb);
    ux_flow_init(0, ux_reset_flow, NULL);
}

#endif  // HAVE_BAGL
