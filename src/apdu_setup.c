/* Tezos Ledger application - Setup APDU instruction handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
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

#include "apdu_setup.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"
#include "ui_setup.h"

#include <string.h>

#define G global.apdu.u.setup

/**
 * @brief Applies the setup
 *
 *        Rounds are also reset to 0
 *
 * @return true
 */
static bool ok(void) {
    copy_bip32_path_with_curve(&(g_hwm.baking_key), &global.path_with_curve);
    g_hwm.main_chain_id = G.main_chain_id;
    g_hwm.hwm.main.highest_level = G.hwm.main;
    g_hwm.hwm.main.highest_round = 0;
    g_hwm.hwm.main.had_attestation = false;
    g_hwm.hwm.main.had_preattestation = false;
    g_hwm.hwm.test.highest_level = G.hwm.test;
    g_hwm.hwm.test.highest_round = 0;
    g_hwm.hwm.test.had_attestation = false;
    g_hwm.hwm.test.had_preattestation = false;

    UPDATE_NVRAM;

    provide_pubkey(&global.path_with_curve);

    return true;
}

/**
 * Cdata:
 *   + (4 bytes) uint32: chain id
 *   + (4 bytes) uint32: main hwm level
 *   + (4 bytes) uint32: test hwm level
 *   + Bip32 path: key path
 */
int handle_setup(buffer_t *cdata, derivation_type_t derivation_type) {
    tz_exc exc = SW_OK;

    TZ_ASSERT_NOT_NULL(cdata);

    TZ_ASSERT(buffer_read_u32(cdata, &G.main_chain_id.v, BE) &&  // chain id
                  buffer_read_u32(cdata, &G.hwm.main, BE) &&     // main hwm level
                  buffer_read_u32(cdata, &G.hwm.test, BE),       // test hwm level
              EXC_WRONG_VALUES);

    TZ_CHECK(read_path_with_curve(derivation_type,
                                  cdata,
                                  &global.path_with_curve,
                                  (cx_ecfp_public_key_t *) &global.public_key));

    TZ_ASSERT(cdata->size == cdata->offset, EXC_WRONG_LENGTH);

    return prompt_setup(ok, reject);

end:
    return io_send_apdu_err(exc);
}

int handle_deauthorize(void) {
    memset(&(g_hwm.baking_key), 0, sizeof(g_hwm.baking_key));
    UPDATE_NVRAM_VAR(baking_key);
#ifdef HAVE_BAGL
    // Ignore calculation errors
    calculate_idle_screen_authorized_key();
    refresh_screens();
#endif  // HAVE_BAGL

    return io_send_sw(SW_OK);
}
