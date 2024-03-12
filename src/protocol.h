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

/// Magic byte values
/// See: https://tezos.gitlab.io/user/key-management.html#signer-requests
#define MAGIC_BYTE_INVALID        0x00u  /// No magic byte
#define MAGIC_BYTE_UNSAFE_OP      0x03u  /// magic byte of an operation
#define MAGIC_BYTE_BLOCK          0x11u  /// magic byte of a block
#define MAGIC_BYTE_PREATTESTATION 0x12u  /// magic byte of a pre-attestation
#define MAGIC_BYTE_ATTESTATION    0x13u  /// magic byte of an attestation

/**
 * @brief Get the magic byte of a data
 *
 * @param data: data
 * @param length: data length
 * @return uint8_t: magic byte result
 */
static inline uint8_t get_magic_byte(uint8_t const *const data, size_t const length) {
    return ((data == NULL) || (length == 0u)) ? MAGIC_BYTE_INVALID : *data;
}

/**
 * @brief Reads unaligned big endian value
 *
 * @param type: type of the value
 * @return in: data to read
 */
#define READ_UNALIGNED_BIG_ENDIAN(type, in)                \
    ({                                                     \
        uint8_t const *bytes = (uint8_t const *) in;       \
        uint8_t out_bytes[sizeof(type)];                   \
        type res;                                          \
                                                           \
        for (size_t i = 0; i < sizeof(type); i++) {        \
            out_bytes[i] = bytes[sizeof(type) - i - 1u];   \
        }                                                  \
        memcpy((uint8_t *) &res, out_bytes, sizeof(type)); \
                                                           \
        res;                                               \
    })

/**
 * @brief Same as READ_UNALIGNED_BIG_ENDIAN but helps keep track of
 *        how many bytes have been read by adding sizeof(type) to the
 *        given counter
 *
 */
#define CONSUME_UNALIGNED_BIG_ENDIAN(counter, type, addr) \
    ({                                                    \
        counter += sizeof(type);                          \
        READ_UNALIGNED_BIG_ENDIAN(type, addr);            \
    })
