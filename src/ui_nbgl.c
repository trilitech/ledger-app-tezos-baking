/* Tezos Ledger application - Common NBGL UI functions

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
#include "bolos_target.h"

#include "ui.h"

#include "baking_auth.h"
#include "exception.h"
#include "globals.h"
#include "glyphs.h"  // ui-menu
#include "keys.h"
#include "memory.h"
#include "os_cx.h"  // ui-menu
#include "to_string.h"

#include <stdbool.h>
#include <string.h>

#include "nbgl_use_case.h"

typedef enum {
    CHAIN_IDX = 0,
    PKH_IDX,
    HWM_IDX,
    VERSION_IDX,
    DEVELOPER_IDX,
    COPYRIGHT_IDX,
    INFO_NB
} tz_infoIndex_t;

static const char* const infoTypes[INFO_NB] =
    {"Chain", "Public Key Hash", "High Watermark", "Version", "Developer", "Copyright"};

#define MAX_LENGTH 200
static const char* infoContents[INFO_NB];
static char infoContentsBridge[INFO_NB][MAX_LENGTH];

enum {
    HWM_ENABLED_TOKEN = FIRST_USER_TOKEN
};
enum {
    HWM_ENABLED_TOKEN_ID = 0,
    SETTINGS_SWITCHES_NB
};

static nbgl_layoutSwitch_t switches[SETTINGS_SWITCHES_NB] = {0};

/**
 * @brief Callback to fill the settings page content
 *
 * @param page: page of the settings
 * @param content: content to fill
 * @return bool: if the page is not out of bounds
 */
static bool navigation_cb_baking(uint8_t page, nbgl_pageContent_t* content) {
    if (page > 2u) {
        return false;
    }

    tz_exc exc = SW_OK;

    for (tz_infoIndex_t idx = 0; idx < INFO_NB; idx++) {
        infoContents[idx] = infoContentsBridge[idx];
    }

    const bool hwm_disabled = g_hwm.hwm_disabled;

    TZ_ASSERT(chain_id_to_string_with_aliases(infoContentsBridge[CHAIN_IDX],
                                              MAX_LENGTH,
                                              &g_hwm.main_chain_id) >= 0,
              EXC_WRONG_LENGTH);

    if (g_hwm.baking_key.bip32_path.length == 0u) {
        TZ_ASSERT(copy_string(infoContentsBridge[PKH_IDX], MAX_LENGTH, "No Key Authorized"),
                  EXC_WRONG_LENGTH);
    } else {
        TZ_CHECK(bip32_path_with_curve_to_pkh_string(infoContentsBridge[PKH_IDX],
                                                     MAX_LENGTH,
                                                     &g_hwm.baking_key));
    }

    TZ_ASSERT(hwm_to_string(infoContentsBridge[HWM_IDX], MAX_LENGTH, &g_hwm.hwm.main) >= 0,
              EXC_WRONG_LENGTH);

    TZ_ASSERT(copy_string(infoContentsBridge[VERSION_IDX], MAX_LENGTH, APPVERSION) >= 0,
              EXC_WRONG_LENGTH);

    TZ_ASSERT(copy_string(infoContentsBridge[DEVELOPER_IDX], MAX_LENGTH, "Ledger") >= 0,
              EXC_WRONG_LENGTH);

    TZ_ASSERT(copy_string(infoContentsBridge[COPYRIGHT_IDX], MAX_LENGTH, "(c) 2023 Ledger") >= 0,
              EXC_WRONG_LENGTH);

    switch (page) {
        case 0:
            content->type = INFOS_LIST;
	    // Only the 3 first info are displayed on the first page
	    // will be removed in the next commit
            content->infosList.nbInfos = 3;
            content->infosList.infoTypes = infoTypes;
            content->infosList.infoContents = (const char* const*) infoContents;
            break;
        case 1:
            switches[HWM_ENABLED_TOKEN_ID].initState = (nbgl_state_t) (!hwm_disabled);
            switches[HWM_ENABLED_TOKEN_ID].text = "High Watermark";
            switches[HWM_ENABLED_TOKEN_ID].subText = "Track high watermark\n in Ledger";
            switches[HWM_ENABLED_TOKEN_ID].token = HWM_ENABLED_TOKEN;
            switches[HWM_ENABLED_TOKEN_ID].tuneId = TUNE_TAP_CASUAL;
            content->type = SWITCHES_LIST;
            content->switchesList.nbSwitches = SETTINGS_SWITCHES_NB;
            content->switchesList.switches = (nbgl_layoutSwitch_t*) switches;
            break;
        case 2:
            content->type = INFOS_LIST;
	    // Only the 3 last info are displayed on the third page
	    // will be removed in the next commit
            content->infosList.nbInfos = 3; // INFO_NB - 3
	    // + 3 because information for page 2 starts at infoTypes[3]
	    // will be removed in the next commit
            content->infosList.infoTypes = infoTypes + 3;
            content->infosList.infoContents = (const char* const*) infoContents + 3;
            break;
        default:
            return false;
    }
    return true;
end:
    TZ_EXC_PRINT(exc);
    return true;
}

static void controls_callback(int token, uint8_t index) {
    UNUSED(index);
    if (token == HWM_ENABLED_TOKEN) {
        toggle_hwm();
    }
}

#define TOTAL_SETTINGS_PAGE  (3)
#define INIT_SETTINGS_PAGE   (0)
#define DISABLE_SUB_SETTINGS false

/**
 * @brief Draws settings pages
 *
 */
static void ui_menu_about_baking(void) {
    nbgl_useCaseSettings("Tezos baking",
                         INIT_SETTINGS_PAGE,
                         TOTAL_SETTINGS_PAGE,
                         DISABLE_SUB_SETTINGS,
                         ui_initial_screen,
                         navigation_cb_baking,
                         controls_callback);
}

#define SETTINGS_BUTTON_ENABLED (true)

void ui_initial_screen(void) {
    nbgl_useCaseHome("Tezos Baking",
                     &C_tezos,
                     NULL,
                     SETTINGS_BUTTON_ENABLED,
                     ui_menu_about_baking,
                     app_exit);
}

#endif  // HAVE_NBGL
