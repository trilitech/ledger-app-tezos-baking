/* Tezos Ledger application - HMAC handler

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
 * @brief Signs with the HMAC protocol a message
 *
 *        The hmac-key is a fixed signed with a key
 *
 * @param cdata: data containing the message and the BIP32 path of the key
 * @param derivation_type: derivation_type of the key
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int handle_hmac(buffer_t *cdata, derivation_type_t derivation_type);
