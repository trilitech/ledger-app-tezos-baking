/* Tezos Ledger application - Delegate NBGL UI handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger

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

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
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

#define MAX_LENGTH 100

/**
 * @brief Index of values for the delegation flow
 */
typedef enum {
    ADDRESS_IDX = 0,
    FEE_IDX,
    DELEGATION_TAG_VALUE_NB
} DelegationTagValueIndex_t;

/**
 * @brief This structure represents a context needed for delegation screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[DELEGATION_TAG_VALUE_NB];
    nbgl_layoutTagValueList_t tagValueList;
    char tagValueRef[DELEGATION_TAG_VALUE_NB][MAX_LENGTH];
} DelegationContext_t;

/// Current delegation context
static DelegationContext_t delegation_context;

/**
 * @brief Callback called when delegation is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("Delegation confirmed", true, ui_initial_screen);
        delegation_context.ok_cb();
    } else {
        nbgl_useCaseStatus("Delegate registration\ncancelled", false, ui_initial_screen);
        delegation_context.cxl_cb();
    }
}

int prompt_delegation(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    tz_exc exc = SW_OK;

    TZ_ASSERT(G.maybe_ops.is_valid, EXC_MEMORY_ERROR);

    delegation_context.ok_cb = ok_cb;
    delegation_context.cxl_cb = cxl_cb;

    TZ_CHECK(bip32_path_with_curve_to_pkh_string(delegation_context.tagValueRef[ADDRESS_IDX],
                                                 MAX_LENGTH,
                                                 &global.path_with_curve));

    TZ_ASSERT(microtez_to_string(delegation_context.tagValueRef[FEE_IDX],
                                 MAX_LENGTH,
                                 G.maybe_ops.v.total_fee) >= 0,
              EXC_WRONG_LENGTH);

    delegation_context.tagValuePair[ADDRESS_IDX].item = "Address";
    delegation_context.tagValuePair[ADDRESS_IDX].value =
        delegation_context.tagValueRef[ADDRESS_IDX];

    delegation_context.tagValuePair[FEE_IDX].item = "Fee";
    delegation_context.tagValuePair[FEE_IDX].value = delegation_context.tagValueRef[FEE_IDX];

    delegation_context.tagValueList.nbPairs = DELEGATION_TAG_VALUE_NB;
    delegation_context.tagValueList.pairs = delegation_context.tagValuePair;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &delegation_context.tagValueList,
                            &C_tezos,
                            "Register delegate",
                            NULL,
                            "Confirm delegate registration",
                            confirmation_callback);
    return 0;

end:
    return io_send_apdu_err(exc);
}

#endif  // HAVE_NBGL
