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

static const char* const infoTypes[] = {"Version", "Developer", "Copyright"};
static const char* const infoContents[] = {VERSION, "Ledger", "(c) 2023 Ledger"};

#define MAX_LENGTH 200
static char* bakeInfoContents[3];
static char buffer[3][MAX_LENGTH];

static const char* const bakeInfoTypes[] = {
    "Chain",
    "Public Key Hash",
    "High Watermark",
};

/**
 * @brief Callback to fill the settings page content
 *
 * @param page: page of the settings
 * @param content: content to fill
 * @return bool: if the page is not out of bounds
 */
static bool navigation_cb_baking(uint8_t page, nbgl_pageContent_t* content) {
    UNUSED(page);

    bakeInfoContents[0] = buffer[0];
    bakeInfoContents[1] = buffer[1];
    bakeInfoContents[2] = buffer[2];

    if (chain_id_to_string_with_aliases(buffer[0], sizeof(buffer[0]), &N_data.main_chain_id) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    if (N_data.baking_key.bip32_path.length == 0u) {
        if (!copy_string(buffer[1], sizeof(buffer[1]), "No Key Authorized")) {
            THROW(EXC_WRONG_LENGTH);
        }
    } else {
        tz_exc exc =
            bip32_path_with_curve_to_pkh_string(buffer[1], sizeof(buffer[1]), &N_data.baking_key);
        if (exc != SW_OK) {
            THROW(exc);
        }
    }

    if (hwm_to_string(buffer[2], sizeof(buffer[2]), &N_data.hwm.main) < 0) {
        THROW(EXC_WRONG_LENGTH);
    }

    if (page == 0u) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 3;
        content->infosList.infoTypes = bakeInfoTypes;
        content->infosList.infoContents = bakeInfoContents;
    } else {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 3;
        content->infosList.infoTypes = infoTypes;
        content->infosList.infoContents = infoContents;
    }

    return true;
}

/**
 * @brief Draws settings pages
 *
 */
static void ui_menu_about_baking(void) {
    nbgl_useCaseSettings("Tezos baking",
                         0,
                         2,
                         false,
                         ui_initial_screen,
                         navigation_cb_baking,
                         NULL);
}

void ui_initial_screen(void) {
    nbgl_useCaseHome("Tezos Baking", &C_tezos, NULL, false, ui_menu_about_baking, exit_app);
}

#endif  // HAVE_NBGL
