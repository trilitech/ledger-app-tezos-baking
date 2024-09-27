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

//  -----------------------------------------------------------
//  -------------------------- INFO ---------------------------
//  -----------------------------------------------------------

typedef enum {
    CHAIN_IDX = 0,
    PKH_IDX,
    HWM_IDX,
    VERSION_IDX,
    DEVELOPER_IDX,
    COPYRIGHT_IDX,
    CONTACT_IDX,
    INFO_NB
} tz_infoIndex_t;

static const char* const infoTypes[INFO_NB] =
    {"Chain", "Public Key Hash", "High Watermark", "Version", "Developer", "Copyright", "Contact"};

#define MAX_LENGTH 200
static const char* infoContents[INFO_NB];
static char infoContentsBridge[INFO_NB][MAX_LENGTH];

static const nbgl_contentInfoList_t infoList = {.nbInfos = INFO_NB,
                                                .infoTypes = infoTypes,
                                                .infoContents = infoContents};

/**
 * @brief Initializes info values
 */
static void initInfo(void) {
    tz_exc exc = SW_OK;

    for (tz_infoIndex_t idx = 0; idx < INFO_NB; idx++) {
        infoContents[idx] = infoContentsBridge[idx];
    }

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

    TZ_ASSERT(
        copy_string(infoContentsBridge[DEVELOPER_IDX], MAX_LENGTH, "Trilitech Kanvas Ltd. et al") >=
            0,
        EXC_WRONG_LENGTH);

    TZ_ASSERT(copy_string(infoContentsBridge[COPYRIGHT_IDX], MAX_LENGTH, "(c) 2024 Trilitech") >= 0,
              EXC_WRONG_LENGTH);
    TZ_ASSERT(
        copy_string(infoContentsBridge[CONTACT_IDX], MAX_LENGTH, "ledger-tezos@trili.tech") >= 0,
        EXC_WRONG_LENGTH);

end:
    TZ_EXC_PRINT(exc);
}

//  -----------------------------------------------------------
//  ------------------------ SETTINGS -------------------------
//  -----------------------------------------------------------

typedef enum {
    HWM_ENABLED_TOKEN = FIRST_USER_TOKEN,
    NEXT_TOKEN
} tz_settingsToken_t;

//  -------------------- SWITCHES SETTINGS --------------------

typedef enum {
    HWM_ENABLED_IDX = 0,
    SETTINGS_SWITCHES_NB
} tz_settingsSwitchesIndex_t;

static nbgl_contentSwitch_t settingsSwitches[SETTINGS_SWITCHES_NB] = {0};

/**
 * @brief Initializes switches settings values
 */
static void initSettingsSwitches(void) {
    nbgl_contentSwitch_t* hwmSwitch = &settingsSwitches[HWM_ENABLED_IDX];
    hwmSwitch->text = "High Watermark";
    hwmSwitch->subText = "Track high watermark\n in Ledger";
    hwmSwitch->initState = (nbgl_state_t) (!g_hwm.hwm_disabled);
    hwmSwitch->token = HWM_ENABLED_TOKEN;
}

/**
 * @brief Callback to controle switches impact
 *
 * @param token: token of the switch toggled
 * @param state: switch state
 * @param page: page index
 */
static void settingsSwitchesToggleCallback(tz_settingsToken_t token, nbgl_state_t state, int page) {
    UNUSED(page);
    if (token == HWM_ENABLED_TOKEN) {
        toggle_hwm();
        settingsSwitches[HWM_ENABLED_IDX].initState = state;
    }
}

#define SETTINGS_SWITCHES_CONTENTS                                                             \
    {                                                                                          \
        .type = SWITCHES_LIST,                                                                 \
        .content.switchesList = {.switches = settingsSwitches,                                 \
                                 .nbSwitches = SETTINGS_SWITCHES_NB},                          \
        .contentActionCallback = (nbgl_contentActionCallback_t) settingsSwitchesToggleCallback \
    }

//  ---------------------- ALL SETTINGS -----------------------

typedef enum {
    SWITCHES_IDX = 0,
    SETTINGS_PAGE_NB
} tz_settingsIndex_t;

static const nbgl_content_t settingsContentsList[SETTINGS_PAGE_NB] = {SETTINGS_SWITCHES_CONTENTS};

static const nbgl_genericContents_t settingsContents = {.callbackCallNeeded = false,
                                                        .contentsList = settingsContentsList,
                                                        .nbContents = SETTINGS_PAGE_NB};

/**
 * @brief Initializes settings values
 */
static void initSettings(void) {
    initSettingsSwitches();
}

//  -----------------------------------------------------------

void ui_initial_screen(void) {
    initSettings();
    initInfo();

    nbgl_useCaseHomeAndSettings("Tezos Baking",
                                &C_tezos,
                                NULL,
                                INIT_HOME_PAGE,
                                &settingsContents,
                                &infoList,
                                NULL,
                                app_exit);
}

#endif  // HAVE_NBGL
