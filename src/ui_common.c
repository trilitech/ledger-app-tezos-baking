/* Tezos Ledger application - Common UI functions

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

#include "ui.h"
#include "os_pin.h"

/**
 * @brief Invalidates the pin to enforce its requirement.
 *
 */
static void require_pin(void) {
    os_global_pin_invalidate();
}

void exit_app(void) {
    require_pin();
    os_sched_exit(-1);
}
