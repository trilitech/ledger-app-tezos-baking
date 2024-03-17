/* Tezos Ledger application - Sign APDU instruction handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
   Copyright 2020 Obsidian Systems

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

#pragma once

#include "apdu.h"

/**
 * @brief Handles SIGN instruction
 *
 * @param cmd: structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_sign(const command_t* cmd, volatile uint32_t* flags);

/**
 * @brief Handles SIGN_WITH_HASH instruction
 *
 * @param cmd: structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_sign_with_hash(const command_t* cmd, volatile uint32_t* flags);
