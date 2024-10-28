/* Tezos Ledger application - Setup NBGL UI handling

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
#include "apdu_setup.h"
#include "ui_setup.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.setup

#define MAX_LENGTH 100

/**
 * @brief Index of values for the setup flow
 */
typedef enum {
    ADDRESS_IDX = 0,
    CHAIN_IDX,
    MAIN_HWM_IDX,
    TEST_HWM_IDX,
    SETUP_TAG_VALUE_NB
} SetupTagValueIndex_t;

/**
 * @brief This structure represents a context needed for setup screens navigation
 *
 */
typedef struct {
    ui_callback_t ok_cb;   /// accept callback
    ui_callback_t cxl_cb;  /// cancel callback
    nbgl_layoutTagValue_t tagValuePair[SETUP_TAG_VALUE_NB];
    nbgl_layoutTagValueList_t tagValueList;
    char tagValueRef[SETUP_TAG_VALUE_NB][MAX_LENGTH];
} SetupContext_t;

/// Current setup context
static SetupContext_t setup_context;

/**
 * @brief Callback called when setup is accepted or cancelled
 *
 * @param confirm: true if accepted, false if cancelled
 */
static void confirmation_callback(bool confirm) {
    if (confirm) {
        setup_context.ok_cb();
        nbgl_useCaseStatus("Setup confirmed", true, ui_initial_screen);
    } else {
        setup_context.cxl_cb();
        nbgl_useCaseStatus("Setup cancelled", false, ui_initial_screen);
    }
}

/**
 * @brief Callback called when setup is cancelled
 *
 */
static void cancel_callback(void) {
    confirmation_callback(false);
}

typedef enum {
    CONFIRM_TOKEN = FIRST_USER_TOKEN
} tz_setupToken_t;

/**
 * @brief Callback called during setup flow
 *
 */
static void setupCallback(tz_setupToken_t token, uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);
    if (token == CONFIRM_TOKEN) {
        confirmation_callback(true);
    }
}

#define SETUP_CONTENT_NB 3

// clang-format off
static const nbgl_content_t setupContentList[SETUP_CONTENT_NB] = {
  {
    .type = CENTERED_INFO,
    .content.centeredInfo = {
      .text1 = "Setup baking",
      .text3 = "Swipe to review",
      .icon  = &C_tezos,
      .style = LARGE_CASE_GRAY_INFO,
    }
  },
  {
    .type = TAG_VALUE_LIST,
    .content.tagValueList = {
      .pairs   = setup_context.tagValuePair,
      .nbPairs = SETUP_TAG_VALUE_NB,
    }
  },
  {
    .type = INFO_BUTTON,
    .content.infoButton = {
      .text        = "Confirm baking setup",
      .icon        = &C_tezos,
      .buttonText  = "Approve",
      .buttonToken = CONFIRM_TOKEN
    },
    .contentActionCallback = (nbgl_contentActionCallback_t) setupCallback
  }
};

static const nbgl_genericContents_t setupContents = {
  .contentsList = setupContentList,
  .nbContents = SETUP_CONTENT_NB
};
// clang-format on

int prompt_setup(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    tz_exc exc = SW_OK;

    setup_context.ok_cb = ok_cb;
    setup_context.cxl_cb = cxl_cb;

    TZ_CHECK(pk_to_pkh_string(setup_context.tagValueRef[ADDRESS_IDX],
                              MAX_LENGTH,
                              (cx_ecfp_public_key_t *) &global.public_key));

    TZ_ASSERT(chain_id_to_string_with_aliases(setup_context.tagValueRef[CHAIN_IDX],
                                              MAX_LENGTH,
                                              &G.main_chain_id) >= 0,
              EXC_WRONG_LENGTH);

    TZ_ASSERT(
        number_to_string(setup_context.tagValueRef[MAIN_HWM_IDX], MAX_LENGTH, G.hwm.main) >= 0,
        EXC_WRONG_LENGTH);

    TZ_ASSERT(
        number_to_string(setup_context.tagValueRef[TEST_HWM_IDX], MAX_LENGTH, G.hwm.test) >= 0,
        EXC_WRONG_LENGTH);

    setup_context.tagValuePair[ADDRESS_IDX].item = "Address";
    setup_context.tagValuePair[ADDRESS_IDX].value = setup_context.tagValueRef[ADDRESS_IDX];

    setup_context.tagValuePair[CHAIN_IDX].item = "Chain";
    setup_context.tagValuePair[CHAIN_IDX].value = setup_context.tagValueRef[CHAIN_IDX];

    setup_context.tagValuePair[MAIN_HWM_IDX].item = "Main Chain HWM";
    setup_context.tagValuePair[MAIN_HWM_IDX].value = setup_context.tagValueRef[MAIN_HWM_IDX];

    setup_context.tagValuePair[TEST_HWM_IDX].item = "Test Chain HWM";
    setup_context.tagValuePair[TEST_HWM_IDX].value = setup_context.tagValueRef[TEST_HWM_IDX];

    setup_context.tagValueList.nbPairs = SETUP_TAG_VALUE_NB;
    setup_context.tagValueList.pairs = setup_context.tagValuePair;

    nbgl_useCaseGenericReview(&setupContents, "Cancel", cancel_callback);

    return 0;

end:
    return io_send_apdu_err(exc);
}

#endif  // HAVE_NBGL
