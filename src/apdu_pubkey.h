/* Tezos Ledger application - Get public key handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
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

/**
 * @brief Gets the public key
 *
 * @param cdata: data containing the BIP32 path of the key
 * @param derivation_type: derivation_type of the key
 * @param authorize: whether to authorize the address or not
 * @param prompt: whether to display address on screen or not
 * @param flags: io flags
 * @return size_t: offset of the apdu response
 */
size_t handle_get_public_key(buffer_t *cdata,
                             derivation_type_t derivation_type,
                             bool authorize,
                             bool prompt,
                             volatile uint32_t *flags);
