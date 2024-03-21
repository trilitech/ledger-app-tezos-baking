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

#include <string.h>

#define TEZOS_HASH_CHECKSUM_SIZE 4u

static void pkh_to_string(char *const buff,
                          size_t const buff_size,
                          signature_type_t const signature_type,
                          uint8_t const hash[HASH_SIZE]);
static size_t microtez_to_string(char *dest, uint64_t number);

void pubkey_to_pkh_string(char *const out,
                          size_t const out_size,
                          derivation_type_t const derivation_type,
                          cx_ecfp_public_key_t const *const public_key) {
    check_null(out);
    check_null(public_key);

    uint8_t hash[HASH_SIZE];
    public_key_hash(hash, sizeof(hash), NULL, derivation_type, public_key);
    pkh_to_string(out, out_size, derivation_type_to_signature_type(derivation_type), hash);
}

void bip32_path_with_curve_to_pkh_string(char *const out,
                                         size_t const out_size,
                                         bip32_path_with_curve_t const *const key) {
    check_null(out);
    check_null(key);

    cx_ecfp_public_key_t pubkey = {0};
    generate_public_key(&pubkey, key->derivation_type, &key->bip32_path);
    pubkey_to_pkh_string(out, out_size, key->derivation_type, &pubkey);
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
 * @param buff: result output
 * @param buff_size: output size
 * @param signature_type: curve of the key
 * @param hash: public key hash
 */
static void pkh_to_string(char *const buff,
                          size_t const buff_size,
                          signature_type_t const signature_type,
                          uint8_t const hash[HASH_SIZE]) {
    check_null(buff);
    check_null(hash);
    if (buff_size < PKH_STRING_SIZE) {
        THROW(EXC_WRONG_LENGTH);
    }

    // Data to encode
    struct __attribute__((packed)) {
        uint8_t prefix[3];
        uint8_t hash[HASH_SIZE];
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
            THROW(EXC_WRONG_PARAM);  // Should not reach
    }

    // hash
    memcpy(data.hash, hash, sizeof(data.hash));
    compute_hash_checksum(data.checksum, &data, sizeof(data) - sizeof(data.checksum));

    if (base58_encode((const uint8_t *) &data, sizeof(data), buff, buff_size) == -1) {
        THROW(EXC_WRONG_LENGTH);
    }
}

/**
 * @brief Converts a chain id to string
 *
 * @param buff: result output
 * @param buff_size: output size
 * @param chain_id: chain id to convert
 */
static void chain_id_to_string(char *const buff,
                               size_t const buff_size,
                               chain_id_t const chain_id) {
    check_null(buff);
    if (buff_size < CHAIN_ID_BASE58_STRING_SIZE) {
        THROW(EXC_WRONG_LENGTH);
    }

    // Must hash big-endian data so treating little endian as big endian just flips
    uint32_t chain_id_value = READ_UNALIGNED_BIG_ENDIAN(uint32_t, &chain_id.v);

    // Data to encode
    struct __attribute__((packed)) {
        uint8_t prefix[3];
        int32_t chain_id;
        uint8_t checksum[TEZOS_HASH_CHECKSUM_SIZE];
    } data = {.prefix = {87, 82, 0}, .chain_id = chain_id_value};

    compute_hash_checksum(data.checksum, &data, sizeof(data) - sizeof(data.checksum));

    if (base58_encode((const uint8_t *) &data, sizeof(data), buff, buff_size) == -1) {
        THROW(EXC_WRONG_LENGTH);
    }
}

#define STRCPY_OR_THROW(buff, size, x, exc) \
    ({                                      \
        if (size < sizeof(x)) {             \
            THROW(exc);                     \
        }                                   \
        strlcpy(buff, x, size);             \
    })

void chain_id_to_string_with_aliases(char *const out,
                                     size_t const out_size,
                                     chain_id_t const *const chain_id) {
    check_null(out);
    check_null(chain_id);
    if (!chain_id->v) {
        STRCPY_OR_THROW(out, out_size, "any", EXC_WRONG_LENGTH);
    } else if (chain_id->v == mainnet_chain_id.v) {
        STRCPY_OR_THROW(out, out_size, "mainnet", EXC_WRONG_LENGTH);
    } else {
        chain_id_to_string(out, out_size, *chain_id);
    }
}

/// These functions do not output terminating null bytes.

/**
 * @brief Converts an uint64 number to string
 *
 *        Fills digits, potentially with all leading zeroes, from the end of the buffer backwards
 *
 *        This is intended to be used with a temporary buffer of length MAX_INT_DIGITS
 *
 * @param dest: result output
 * @param number: number to convert
 * @param leading_zeroes: if keep the leading 0
 * @return size_t: offset of where it stopped filling in
 */
static inline size_t convert_number(char dest[MAX_INT_DIGITS],
                                    uint64_t number,
                                    bool leading_zeroes) {
    check_null(dest);

    uint64_t rest = number;
    size_t i = MAX_INT_DIGITS;
    do {
        i--;
        dest[i] = '0' + (rest % 10u);
        rest /= 10u;
    } while ((i > 0u) && (leading_zeroes || (rest > 0u)));
    return i;
}

void number_to_string_indirect32(char *const dest,
                                 size_t const buff_size,
                                 uint32_t const *const number) {
    check_null(dest);
    check_null(number);
    if (buff_size < (MAX_INT_DIGITS + 1u)) {
        THROW(EXC_WRONG_LENGTH);  // terminating null
    }
    number_to_string(dest, *number);
}

void microtez_to_string_indirect(char *const dest,
                                 size_t const buff_size,
                                 uint64_t const *const number) {
    check_null(dest);
    check_null(number);
    if (buff_size < MAX_INT_DIGITS + sizeof(TICKER_WITH_SPACE) + 1u) {
        THROW(EXC_WRONG_LENGTH);  // + terminating null + decimal point
    }
    microtez_to_string(dest, *number);
}

size_t number_to_string(char *const dest, uint64_t number) {
    check_null(dest);
    char tmp[MAX_INT_DIGITS];
    size_t off = convert_number(tmp, number, false);

    // Copy without leading 0s
    size_t length = sizeof(tmp) - off;
    memcpy(dest, tmp + off, length);
    dest[length] = '\0';
    return length;
}

/// Microtez are in millionths
#define TEZ_SCALE      1000000u
#define DECIMAL_DIGITS 6u

/**
 * @brief Converts an uint64 number to microtez as string
 *
 *        These functions output terminating null bytes, and return the ending offset.
 *
 * @param dest: output buffer
 * @param number: number to convert
 * @return size_t: size of the result
 */
static size_t microtez_to_string(char *const dest, uint64_t number) {
    check_null(dest);
    uint64_t whole_tez = number / TEZ_SCALE;
    uint64_t fractional = number % TEZ_SCALE;
    size_t off = number_to_string(dest, whole_tez);
    if (fractional == 0u) {
        // Append the ticker at the end of the amount.
        memcpy(dest + off, TICKER_WITH_SPACE, sizeof(TICKER_WITH_SPACE));
        off += sizeof(TICKER_WITH_SPACE);

        return off;
    }
    dest[off] = '.';
    off++;

    char tmp[MAX_INT_DIGITS];
    convert_number(tmp, number, true);

    // Eliminate trailing 0s
    char *start = tmp + MAX_INT_DIGITS - DECIMAL_DIGITS;
    char *end;
    for (end = tmp + MAX_INT_DIGITS - 1u; end >= start; end--) {
        if (*end != '0') {
            end++;
            break;
        }
    }

    size_t length = end - start;
    memcpy(dest + off, start, length);
    off += length;

    // Append the ticker at the end of the amount.
    memcpy(dest + off, TICKER_WITH_SPACE, sizeof(TICKER_WITH_SPACE));
    off += sizeof(TICKER_WITH_SPACE);

    return off;
}

void copy_string(char *const dest, size_t const buff_size, char const *const src) {
    check_null(dest);
    check_null(src);
    char const *const src_in = (char const *) PIC(src);
    // I don't care that we will loop through the string twice, latency is not an issue
    if (strlen(src_in) >= buff_size) {
        THROW(EXC_WRONG_LENGTH);
    }
    strlcpy(dest, src_in, buff_size);
}
