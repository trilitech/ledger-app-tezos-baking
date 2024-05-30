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

#pragma once

#include "apdu.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Handles baking information setup
 *
 *        Asks user to check the setup
 *
 * @param cdata: data containing the chain_id, the two HWM and the
 *               BIP32 path of the key
 * @param derivation_type: derivation_type of the key
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int handle_setup(buffer_t *cdata, derivation_type_t derivation_type);

/**
 * @brief Handles deauthorize
 *
 *        Deauthorizes the authorized key
 *
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int handle_deauthorize(void);
