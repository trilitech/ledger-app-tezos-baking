/* Tezos Ledger application - Exception primitives

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2019 Ledger
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

#include "os.h"

// Throw this to indicate prompting
#define ASYNC_EXCEPTION 0x2000

/**
 * @brief Standard APDU error codes
 *
 *        https://www.eftlab.co.uk/index.php/site-map/knowledge-base/118-apdu-response-list
 *
 */

typedef uint16_t tz_exc;

#define SW_OK                         0x9000u
#define EXC_WRONG_PARAM               0x6B00u
#define EXC_WRONG_LENGTH              0x6C00u
#define EXC_INVALID_INS               0x6D00u
#define EXC_WRONG_LENGTH_FOR_INS      0x917Eu
#define EXC_REJECT                    0x6985u
#define EXC_PARSE_ERROR               0x9405u
#define EXC_REFERENCED_DATA_NOT_FOUND 0x6A88u
#define EXC_WRONG_VALUES              0x6A80u
#define EXC_SECURITY                  0x6982u
#define EXC_CLASS                     0x6E00u
#define EXC_MEMORY_ERROR              0x9200u
#define EXC_UNKNOWN_CX_ERR            0x9001u

/**
 * @brief Checks if a pointer is NULL
 *
 *        Crashes can be harder to debug than exceptions and latency isn't a big concern
 *
 * @param ptr: pointer
 */
static inline void check_null(void volatile const *const ptr) {
    if (ptr == NULL) {
        THROW(EXC_MEMORY_ERROR);
    }
}

// Checks the error code of a function
#define TZ_CHECK(call)      \
    do {                    \
        exc = (call);       \
        if (exc != SW_OK) { \
            goto end;       \
        }                   \
    } while (0)

// Asserts a condition. Updates `exc` accordingly
#define TZ_ASSERT(cond, _exc) \
    do {                      \
        if (!(cond)) {        \
            exc = (_exc);     \
            goto end;         \
        }                     \
    } while (0)

#define TZ_ASSERT_NOT_NULL(_x) TZ_ASSERT((_x) != NULL, EXC_MEMORY_ERROR)

// Converts `cx_err_t error` to `tz_exc exc`
#define TZ_CONVERT_CX()                        \
    do {                                       \
        if (error != CX_OK) {                  \
            PRINTF("CX error: 0x%08x", error); \
            exc = EXC_UNKNOWN_CX_ERR;          \
        }                                      \
    } while (0)
