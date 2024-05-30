/* Tezos Ledger application - Sign handling

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
 * @brief Selects the key with which the message will be signed
 *
 * @param cdata: data containing the BIP32 path of the key
 * @param derivation_type: derivation_type of the key
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int select_signing_key(buffer_t *cdata, derivation_type_t derivation_type);

/**
 * @brief Parse and signs a message
 *
 * @param cdata: data containing the message to sign
 * @param last: whether the part of the message is the last one or not
 * @param with_hash: whether the hash of the message is requested or not
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int handle_sign(buffer_t *cdata, bool last, bool with_hash);
