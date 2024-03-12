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

// Standard APDU error codes:
// https://www.eftlab.co.uk/index.php/site-map/knowledge-base/118-apdu-response-list

#define EXC_WRONG_PARAM               0x6B00
#define EXC_WRONG_LENGTH              0x6C00
#define EXC_INVALID_INS               0x6D00
#define EXC_WRONG_LENGTH_FOR_INS      0x917E
#define EXC_REJECT                    0x6985
#define EXC_PARSE_ERROR               0x9405
#define EXC_REFERENCED_DATA_NOT_FOUND 0x6A88
#define EXC_WRONG_VALUES              0x6A80
#define EXC_SECURITY                  0x6982
#define EXC_CLASS                     0x6E00
#define EXC_MEMORY_ERROR              0x9200

// Crashes can be harder to debug than exceptions and latency isn't a big concern
static inline void check_null(void volatile const *const ptr) {
    if (ptr == NULL) {
        THROW(EXC_MEMORY_ERROR);
    }
}
