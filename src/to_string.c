/* Tezos Ledger application - Data to string functions

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

#include "to_string.h"

#include <base58.h>

#include "apdu.h"
#include "keys.h"
#include "read.h"

#include <string.h>

#define TEZOS_HASH_CHECKSUM_SIZE 4u

static int pkh_to_string(char *const dest,
                         size_t const dest_size,
                         signature_type_t const signature_type,
                         uint8_t const hash[KEY_HASH_SIZE]);

tz_exc bip32_path_with_curve_to_pkh_string(char *const out,
                                           size_t const out_size,
                                           bip32_path_with_curve_t const *const key) {
    tz_exc exc = SW_OK;
    cx_err_t error = CX_OK;
    uint8_t hash[KEY_HASH_SIZE];

    TZ_ASSERT_NOT_NULL(out);
    TZ_ASSERT_NOT_NULL(key);

    CX_CHECK(generate_public_key_hash(hash, sizeof(hash), NULL, key));

    TZ_ASSERT(pkh_to_string(out,
                            out_size,
                            derivation_type_to_signature_type(key->derivation_type),
                            hash) >= 0,
              EXC_WRONG_LENGTH);

end:
    TZ_CONVERT_CX();
    return exc;
}

/**
 * @brief Computes the ckecsum of a hash
 *
 * @param out: result output
 * @param data: hash input
 * @param size: input size
 */
static void compute_hash_checksum(uint8_t out[TEZOS_HASH_CHECKSUM_SIZE],
                                  void const *const data,
                                  size_t size) {
    uint8_t checksum[CX_SHA256_SIZE];
    cx_hash_sha256(data, size, checksum, sizeof(checksum));
    cx_hash_sha256(checksum, sizeof(checksum), checksum, sizeof(checksum));
    memcpy(out, checksum, TEZOS_HASH_CHECKSUM_SIZE);
}

/**
 * @brief Converts a public key hash to string
 *
 * @param dest: result output
 * @param dest_size: output size
 * @param signature_type: curve of the key
 * @param hash: public key hash
 * @return int: size of the result, negative integer on failure
 */
static int pkh_to_string(char *const dest,
                         size_t const dest_size,
                         signature_type_t const signature_type,
                         uint8_t const hash[KEY_HASH_SIZE]) {
    if ((dest == NULL) || (hash == NULL)) {
        return -1;
    }

    // Data to encode
    struct __attribute__((packed)) {
        uint8_t prefix[3];
        uint8_t hash[KEY_HASH_SIZE];
        uint8_t checksum[TEZOS_HASH_CHECKSUM_SIZE];
    } data;

    // prefix
    switch (signature_type) {
        case SIGNATURE_TYPE_UNSET:
            data.prefix[0] = 2u;
            data.prefix[1] = 90u;
            data.prefix[2] = 121u;
            break;
        case SIGNATURE_TYPE_ED25519:
            data.prefix[0] = 6u;
            data.prefix[1] = 161u;
            data.prefix[2] = 159u;
            break;
        case SIGNATURE_TYPE_SECP256K1:
            data.prefix[0] = 6u;
            data.prefix[1] = 161u;
            data.prefix[2] = 161u;
            break;
        case SIGNATURE_TYPE_SECP256R1:
            data.prefix[0] = 6u;
            data.prefix[1] = 161u;
            data.prefix[2] = 164u;
            break;
        default:
            return -1;
    }

    // hash
    memcpy(data.hash, hash, sizeof(data.hash));
    compute_hash_checksum(data.checksum, &data, sizeof(data) - sizeof(data.checksum));

    return base58_encode((const uint8_t *) &data, sizeof(data), dest, dest_size);
}

/**
 * @brief Converts a chain id to string
 *
 * @param dest: result output
 * @param dest_size: output size
 * @param chain_id: chain id to convert
 */
static int chain_id_to_string(char *const dest,
                              size_t const dest_size,
                              chain_id_t const *const chain_id) {
    if ((dest == NULL) || (chain_id == NULL)) {
        return -1;
    }

    // Must hash big-endian data so treating little endian as big endian just flips
    uint32_t chain_id_value = read_u32_be((const uint8_t *) &chain_id->v, 0);

    // Data to encode
    struct __attribute__((packed)) {
        uint8_t prefix[3];
        int32_t chain_id;
        uint8_t checksum[TEZOS_HASH_CHECKSUM_SIZE];
    } data = {.prefix = {87, 82, 0}, .chain_id = chain_id_value};

    compute_hash_checksum(data.checksum, &data, sizeof(data) - sizeof(data.checksum));

    return base58_encode((const uint8_t *) &data, sizeof(data), dest, dest_size);
}

#define SAFE_STRCPY(dest, dest_size, in) \
    ({                                   \
        size_t in_size = strlen(in);     \
        if (dest_size < in_size) {       \
            return -1;                   \
        }                                \
        strlcpy(dest, in, dest_size);    \
        return in_size;                  \
    })

int chain_id_to_string_with_aliases(char *const dest,
                                    size_t const dest_size,
                                    chain_id_t const *const chain_id) {
    if ((dest == NULL) || (chain_id == NULL)) {
        return -1;
    }

    if (!chain_id->v) {
        SAFE_STRCPY(dest, dest_size, "any");
    } else if (chain_id->v == mainnet_chain_id.v) {
        SAFE_STRCPY(dest, dest_size, "mainnet");
    } else {
        return chain_id_to_string(dest, dest_size, chain_id);
    }
}

/**
 * @brief Converts an uint64 number to string
 *
 *        Fills digits, potentially with all leading zeroes, from the end of the buffer backwards
 *
 *        This is intended to be used with a temporary buffer of length MAX_INT_DIGITS
 *
 * @param dest: result output
 * @param dest_size: output size
 * @param number: number to convert
 * @param leading_zeroes: if keep the leading 0
 * @return int: offset of where it stopped filling in, negative integer on failure
 */
static inline int convert_number(char *dest,
                                 size_t dest_size,
                                 uint64_t number,
                                 bool leading_zeroes) {
    if ((dest == NULL) || (dest_size == 0u)) {
        return -1;
    }

    uint64_t rest = number;
    size_t i = dest_size;
    do {
        i--;
        dest[i] = '0' + (rest % 10u);
        rest /= 10u;
    } while ((i > 0u) && (leading_zeroes || (rest > 0u)));
    if (rest > 0u) {
        return -1;
    }
    return i;
}

int number_to_string(char *const dest, size_t dest_size, uint64_t number) {
    if (dest == NULL) {
        return -1;
    }

    char tmp[MAX_INT_DIGITS];
    int result = convert_number(tmp, sizeof(tmp), number, false);
    if (result < 0) {
        return result;
    }
    size_t offset = (size_t) result;

    // Copy without leading 0s
    size_t length = sizeof(tmp) - offset;
    if (dest_size < (length + 1u)) {
        return -1;
    }

    memcpy(dest, tmp + offset, length);
    dest[length] = '\0';
    return length;
}

/// Microtez are in millionths
#define TEZ_SCALE      1000000u
#define DECIMAL_DIGITS 6u

int microtez_to_string(char *const dest, size_t dest_size, uint64_t number) {
    if (dest == NULL) {
        return -1;
    }

    uint64_t whole_tez = number / TEZ_SCALE;
    uint64_t fractional = number % TEZ_SCALE;

    int result = number_to_string(dest, dest_size, whole_tez);
    if (result < 0) {
        return result;
    }
    size_t offset = (size_t) result;

    if (fractional != 0u) {
        dest[offset] = '.';
        offset++;

        char tmp[MAX_INT_DIGITS];
        result = convert_number(tmp, sizeof(tmp), number, true);
        if (result < 0) {
            return result;
        }

        // Eliminate trailing 0s
        char *start = tmp + sizeof(tmp) - DECIMAL_DIGITS;
        size_t length = DECIMAL_DIGITS;
        while ((length > 0u) && (start[length - 1u] == '0')) {
            length--;
        }

        if ((dest_size - offset) < length) {
            return -1;
        }

        memcpy(dest + offset, start, length);
        offset += length;
    }

    if ((dest_size - offset) < sizeof(TICKER_WITH_SPACE)) {
        return -1;
    }

    // Append the ticker at the end of the amount.
    memcpy(dest + offset, TICKER_WITH_SPACE, sizeof(TICKER_WITH_SPACE));
    offset += sizeof(TICKER_WITH_SPACE);

    return offset;
}

int hwm_to_string(char *dest, size_t dest_size, high_watermark_t const *const hwm) {
    if ((dest == NULL) || (hwm == NULL)) {
        return -1;
    }
    int result = number_to_string(dest, dest_size, hwm->highest_level);
    if (result < 0) {
        return result;
    }
    size_t offset = (size_t) result;

    dest[offset] = ' ';
    offset++;

    result = number_to_string(dest + offset, dest_size - offset, hwm->highest_round);
    if (result < 0) {
        return result;
    }

    return offset + (size_t) result;
}

int hwm_status_to_string(char *dest, size_t dest_size, volatile bool const *hwm_disabled) {
    if ((dest == NULL) || (dest_size < 9u) || (hwm_disabled == NULL)) {
        return -1;
    }
    memcpy(dest, *hwm_disabled ? "Disabled" : "Enabled", dest_size);
    return dest_size;
}

int copy_string(char *const dest, size_t const dest_size, char const *const src) {
    if ((dest == NULL) || (src == NULL)) {
        return -1;
    }
    char const *const src_in = (char const *) PIC(src);
    SAFE_STRCPY(dest, dest_size, src_in);
}
