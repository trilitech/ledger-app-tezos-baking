/* Tezos Ledger application - Memory primitives

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2021 Ledger
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

#include <string.h>

#include "exception.h"

/**
 * @brief Compares two buffer
 *
 *        Statically guarantees that buffer sizes match
 *
 * @param a: first buffer
 * @param b: second buffer
 */
#define COMPARE(a, b)                                                         \
    ({                                                                        \
        _Static_assert(sizeof(*a) == sizeof(*b), "Size mismatch in COMPARE"); \
        check_null(a);                                                        \
        check_null(b);                                                        \
        memcmp((const uint8_t *) a, (const uint8_t *) b, sizeof(*a));         \
    })

/**
 * @brief Returns the number of element of an array
 *
 * @param a: array
 */
#define NUM_ELEMENTS(a) (sizeof(a) / sizeof(*a))
