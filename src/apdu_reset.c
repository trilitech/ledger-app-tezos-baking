/* Tezos Ledger application - Reset APDU instruction handling

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
#include "apdu_reset.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "ui_reset.h"

#define G global.apdu.u.baking

/**
 * @brief Resets main and test level
 *
 *        Rounds are also reset to 0
 *
 * @return true
 */
bool ok(void) {
    UPDATE_NVRAM(ram, {
        ram->hwm.main.highest_level = G.reset_level;
        ram->hwm.main.highest_round = 0;
        ram->hwm.main.had_attestation = false;
        ram->hwm.test.highest_level = G.reset_level;
        ram->hwm.test.highest_round = 0;
        ram->hwm.test.had_attestation = false;
    });

    // Send back the response, do not restart the event loop
    delayed_send(finalize_successful_send(0));
    return true;
}

size_t handle_apdu_reset(__attribute__((unused)) uint8_t instruction, volatile uint32_t* flags) {
    uint8_t* dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;
    uint32_t dataLength = G_io_apdu_buffer[OFFSET_LC];
    if (dataLength != sizeof(level_t)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }
    level_t const lvl = READ_UNALIGNED_BIG_ENDIAN(level_t, dataBuffer);
    if (!is_valid_level(lvl)) {
        THROW(EXC_PARSE_ERROR);
    }

    G.reset_level = lvl;
    prompt_reset(ok, delay_reject);
    *flags = IO_ASYNCH_REPLY;
    return 0;
}
