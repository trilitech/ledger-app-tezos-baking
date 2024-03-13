/* Tezos Ledger application - Protocols primitives

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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "os.h"
#include "cx.h"
#include "types.h"

#define MAGIC_BYTE_INVALID        0x00
#define MAGIC_BYTE_UNSAFE_OP      0x03
#define MAGIC_BYTE_BLOCK          0x11
#define MAGIC_BYTE_PREATTESTATION 0x12
#define MAGIC_BYTE_ATTESTATION    0x13

static inline uint8_t get_magic_byte(uint8_t const *const data, size_t const length) {
    return (data == NULL || length == 0) ? MAGIC_BYTE_INVALID : *data;
}

#define READ_UNALIGNED_BIG_ENDIAN(type, in)             \
    ({                                                  \
        uint8_t const *bytes = (uint8_t const *) in;    \
        uint8_t out_bytes[sizeof(type)];                \
        type res;                                       \
                                                        \
        for (size_t i = 0; i < sizeof(type); i++) {     \
            out_bytes[i] = bytes[sizeof(type) - i - 1]; \
        }                                               \
        memcpy(&res, out_bytes, sizeof(type));          \
                                                        \
        res;                                            \
    })

// Same as READ_UNALIGNED_BIG_ENDIAN but helps keep track of how many bytes
// have been read by adding sizeof(type) to the given counter.
#define CONSUME_UNALIGNED_BIG_ENDIAN(counter, type, addr) \
    ({                                                    \
        counter += sizeof(type);                          \
        READ_UNALIGNED_BIG_ENDIAN(type, addr);            \
    })
