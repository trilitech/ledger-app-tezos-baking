/* Tezos Ledger application - Query APDU instruction handling

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

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Handles QUERY_AUTH instruction
 *
 *        Fills apdu response with the authorized key path
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_query_auth_key(uint8_t instruction, volatile uint32_t* flags);

/**
 * @brief Handles QUERY_AUTH_KEY_WITH_CURVE instruction
 *
 *        Fills apdu response with the authorized key path and its curve
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_query_auth_key_with_curve(uint8_t instruction, volatile uint32_t* flags);

/**
 * @brief Handles QUERY_MAIN_HWM instruction
 *
 *        Fills apdu response with main HWM
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_main_hwm(uint8_t instruction, volatile uint32_t* flags);

/**
 * @brief Handles QUERY_ALL_HWM instruction
 *
 *        Fills apdu response with main HWM, test HWM and main chain id
 *
 * @param instruction: apdu instruction
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_apdu_all_hwm(uint8_t instruction, volatile uint32_t* flags);
