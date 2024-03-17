/* Tezos Ledger application - Query handling

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
 * @brief Get the authorized key BIP32 path
 *
 * @return size_t: offset of the apdu response
 */
size_t handle_query_auth_key(void);

/**
 * @brief Get the authorized key BIP32 path and its curve
 *
 * @return size_t: offset of the apdu response
 */
size_t handle_query_auth_key_with_curve(void);

/**
 * @brief Get the main HWM
 *
 * @return size_t: offset of the apdu response
 */
size_t handle_query_main_hwm(void);

/**
 * @brief Get the main chain id, the main HWM and the test HWM
 *
 * @return size_t: offset of the apdu response
 */
size_t handle_query_all_hwm(void);
