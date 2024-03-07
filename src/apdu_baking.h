/* Tezos Ledger application - Baking APDU instruction handling

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
#include <stdbool.h>

size_t handle_apdu_query_auth_key(uint8_t instruction, volatile uint32_t* flags);
size_t handle_apdu_query_auth_key_with_curve(uint8_t instruction, volatile uint32_t* flags);
size_t handle_apdu_main_hwm(uint8_t instruction, volatile uint32_t* flags);
size_t handle_apdu_all_hwm(uint8_t instruction, volatile uint32_t* flags);
