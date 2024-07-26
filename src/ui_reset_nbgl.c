/* Tezos Ledger application - Reset BAGL UI handling

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

#include "ui_reset.h"

#include "apdu_reset.h"
#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#include "nbgl_use_case.h"
#define G global.apdu.u.baking

#define MAX_LENGTH (MAX_INT_DIGITS + 1u)

/**
 * @brief Index of values for the reset flow
 */
typedef enum {
    LEVEL_IDX = 0,
    RESET_TAG_VALUE_NB
} ResetTagValueIndex_t;

/**
 * @brief This structure represents a context needed for reset screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[RESET_TAG_VALUE_NB];
    nbgl_layoutTagValueList_t tagValueList;
    char tagValueRef[RESET_TAG_VALUE_NB][MAX_LENGTH];
} ResetContext_t;

/// Current reset context
static ResetContext_t reset_context;

/**
 * @brief Callback called when reset is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        reset_context.ok_cb();
        nbgl_useCaseStatus("Reset confirmed", true, ui_initial_screen);
    } else {
        reset_context.cxl_cb();
        nbgl_useCaseStatus("Reset cancelled", false, ui_initial_screen);
    }
}

/**
 * @brief Callback called when reset is cancelled
 *
 */
static void cancel_callback(void) {
    confirmation_callback(false);
}

typedef enum {
    CONFIRM_TOKEN = FIRST_USER_TOKEN
} tz_resetToken_t;

/**
 * @brief Callback called during reset flow
 *
 */
static void resetCallback(tz_resetToken_t token, uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);
    if (token == CONFIRM_TOKEN) {
        confirmation_callback(true);
    }
}

#define RESET_CONTENT_NB 3

// clang-format off
static const nbgl_content_t resetContentList[RESET_CONTENT_NB] = {
  {
    .type = CENTERED_INFO,
    .content.centeredInfo = {
      .text1 = "Reset HWM",
      .text3 = "Swipe to review",
      .icon  = &C_tezos,
      .style = LARGE_CASE_GRAY_INFO,
    }
  },
  {
    .type = TAG_VALUE_LIST,
    .content.tagValueList = {
      .pairs   = reset_context.tagValuePair,
      .nbPairs = RESET_TAG_VALUE_NB,
    }
  },
  {
    .type = INFO_BUTTON,
    .content.infoButton = {
      .text        = "Confirm HWM reset",
      .icon        = &C_tezos,
      .buttonText  = "Approve",
      .buttonToken = CONFIRM_TOKEN
    },
    .contentActionCallback = (nbgl_contentActionCallback_t) resetCallback
  }
};

static const nbgl_genericContents_t resetContents = {
  .contentsList = resetContentList,
  .nbContents = RESET_CONTENT_NB
};
// clang-format on

int prompt_reset(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    tz_exc exc = SW_OK;

    reset_context.ok_cb = ok_cb;
    reset_context.cxl_cb = cxl_cb;

    TZ_ASSERT(
        number_to_string(reset_context.tagValueRef[LEVEL_IDX], MAX_LENGTH, G.reset_level) >= 0,
        EXC_WRONG_LENGTH);

    reset_context.tagValuePair[LEVEL_IDX].item = "Reset level";
    reset_context.tagValuePair[LEVEL_IDX].value = reset_context.tagValueRef[LEVEL_IDX];

    reset_context.tagValueList.nbPairs = RESET_TAG_VALUE_NB;
    reset_context.tagValueList.pairs = reset_context.tagValuePair;

    nbgl_useCaseGenericReview(&resetContents, "Cancel", cancel_callback);

    return 0;

end:
    return io_send_apdu_err(exc);
}

#endif  // HAVE_NBGL
