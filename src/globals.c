/* Tezos Ledger application - Global strcuture handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2022 Nomadic Labs <contact@nomadic-labs.com>
   Copyright 2021 Ledger
   Copyright 2021 Obsidian Systems

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

#include "globals.h"

#include "exception.h"
#include "to_string.h"

#include "ux.h"

#include <string.h>

// WARNING: ***************************************************
// Non-const globals MUST NOT HAVE AN INITIALIZER.
//
// Providing an initializer will cause the application to crash
// if you write to it.
// ************************************************************

globals_t global;

void clear_apdu_globals(void) {
    memset(&global.apdu, 0, sizeof(global.apdu));
}

void init_globals(void) {
    memset(&global, 0, sizeof(global));
    memcpy(&g_hwm, (const void *) (&N_data), sizeof(g_hwm));
}

void toggle_hwm(void) {
    g_hwm.hwm_disabled = !(g_hwm.hwm_disabled);
    UPDATE_NVRAM;  // Update the NVRAM data.
}

// DO NOT TRY TO INIT THIS. This can only be written via an system call.
// The "N_" is *significant*. It tells the linker to put this in NVRAM.
baking_data const N_data_real;

high_watermark_t *select_hwm_by_chain(chain_id_t const chain_id) {
    return ((chain_id.v == g_hwm.main_chain_id.v) || !g_hwm.main_chain_id.v) ? &g_hwm.hwm.main
                                                                             : &g_hwm.hwm.test;
}
