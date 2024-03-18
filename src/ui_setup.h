/* Tezos Ledger application - Setup UI handling

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

#include "types.h"

/**
 * @brief Draws setup confirmation pages flow
 *
 *        - Initial screen
 *        - Values:
 *          - Address
 *          - Chain id
 *          - main HWM level
 *          - test HWM level
 *        - Confirmation screens
 *
 * @param ok_cb: accept callback
 * @param cxl_cb: reject callback
 * @return int: zero or positive integer if success, negative integer otherwise.
 */
int prompt_setup(ui_callback_t const ok_cb, ui_callback_t const cxl_cb);
