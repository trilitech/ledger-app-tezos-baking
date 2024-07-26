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
#include "to_string.h"
#include "ui.h"
#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

/**
 * @brief This structure represents a context needed for delegation screens navigation
 *
 */
typedef struct {
    char address[PKH_STRING_SIZE];
    char fee[MAX_INT_DIGITS + sizeof(TICKER_WITH_SPACE) + 1u];
} DelegationContext_t;

/// Current delegation context
static DelegationContext_t delegation_context;

UX_STEP_NOCB(ux_register_step, bnnn_paging, {"Register", "as delegate?"});
UX_STEP_NOCB(ux_delegate_step, bnnn_paging, {"Address", delegation_context.address});
UX_STEP_NOCB(ux_fee_step, bnnn_paging, {"Fee", delegation_context.fee});

UX_CONFIRM_FLOW(ux_delegation_flow, &ux_register_step, &ux_delegate_step, &ux_fee_step);

int prompt_delegation(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    tz_exc exc = SW_OK;

    TZ_ASSERT(G.maybe_ops.is_valid, EXC_MEMORY_ERROR);

    memset(&delegation_context, 0, sizeof(delegation_context));

    TZ_CHECK(bip32_path_with_curve_to_pkh_string(delegation_context.address,
                                                 sizeof(delegation_context.address),
                                                 &global.path_with_curve));

    TZ_ASSERT(microtez_to_string(delegation_context.fee,
                                 sizeof(delegation_context.fee),
                                 G.maybe_ops.v.total_fee) >= 0,
              EXC_WRONG_LENGTH);

    ux_prepare_confirm_callbacks(ok_cb, cxl_cb);
    ux_flow_init(0, ux_delegation_flow, NULL);
    return 0;

end:
    return io_send_apdu_err(exc);
}

#endif  // HAVE_BAGL
