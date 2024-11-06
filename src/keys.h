/* Tezos Ledger application - Key handling

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

#pragma once

#include "bip32.h"
#include "buffer.h"
#include "os_cx.h"

#ifndef TARGET_NANOS
#include "crypto.h"
#endif

/***** Key Type *****/

/**
 * NOTE: There are *two* ways that "key type" or "curve code" are represented in
 * this code base:
 *   1. `derivation_type` represents how a key will be derived from the seed. It
 *      is almost the same as `signature_type` but allows for multiple derivation
 *      strategies for ed25519. This type is often parsed from the APDU
 *      instruction. See `parse_derivation_type` for the mapping.
 *   2. `signature_type` represents how a key will be used for signing.
 *      The mapping from `derivation_type` to `signature_type` is injective.
 *      See `derivation_type_to_signature_type`.
 *      This type is parsed from Tezos data headers. See the relevant parsing
 *      code for the mapping.
 */
typedef enum {
    DERIVATION_TYPE_ED25519 = 0x00,
    DERIVATION_TYPE_SECP256K1 = 0x01,
    DERIVATION_TYPE_SECP256R1 = 0x02,
    DERIVATION_TYPE_BIP32_ED25519 = 0x03,
#ifdef TARGET_NANOS
    DERIVATION_TYPE_UNSET = 0x04
#else
    DERIVATION_TYPE_BLS12_381 = 0x04,
    DERIVATION_TYPE_UNSET = 0x05
#endif
} derivation_type_t;

typedef enum {
    SIGNATURE_TYPE_ED25519 = 0,
    SIGNATURE_TYPE_SECP256K1 = 1,
    SIGNATURE_TYPE_SECP256R1 = 2,
#ifdef TARGET_NANOS
    SIGNATURE_TYPE_UNSET = 3
#else
    SIGNATURE_TYPE_BLS12_381 = 3,
    SIGNATURE_TYPE_UNSET = 4
#endif
} signature_type_t;

#define DERIVATION_TYPE_IS_SET(type) \
    (((derivation_type_t) 0 <= type) && (type < DERIVATION_TYPE_UNSET))

#define SIGNATURE_TYPE_IS_SET(type) \
    (((signature_type_t) 0 <= type) && (type < SIGNATURE_TYPE_UNSET))

/**
 * @brief Gets the signature_type of a public_key
 *
 * @param public_key: public key
 * @return signature_type_t: signature_type result
 */
static inline signature_type_t signature_type_of_public_key(
    cx_ecfp_public_key_t const *const public_key) {
    switch (public_key->curve) {
        case CX_CURVE_SECP256K1:
            return SIGNATURE_TYPE_SECP256K1;
        case CX_CURVE_SECP256R1:
            return SIGNATURE_TYPE_SECP256R1;
        case CX_CURVE_Ed25519:
            return SIGNATURE_TYPE_ED25519;
#ifndef TARGET_NANOS
        case CX_CURVE_BLS12_381_G1:
            return SIGNATURE_TYPE_BLS12_381;
#endif
        default:
            return SIGNATURE_TYPE_UNSET;
    }
}

/***** Bip32 path *****/

/**
 * @brief This structure represents a bip32 path
 *
 */
typedef struct {
    uint8_t length;                       ///< length of the path
    uint32_t components[MAX_BIP32_PATH];  ///< array of components
} bip32_path_t;

/**
 * @brief This structure represents a bip32 path and its curve
 *
 *        Defines the key when associated with a mnemonic seed.
 *
 */
typedef struct {
    bip32_path_t bip32_path;            ///< bip32 path of the key
    derivation_type_t derivation_type;  ///< curve of the key
} bip32_path_with_curve_t;

/**
 * @brief Reads a bip32_path
 *
 *        Data:
 *          + (1 byte) uint8: length (!= 0)
 *          + (length bytes) list: (4 bytes) uint32: component
 *
 * @param buf: input buffer
 * @param out: bip32_path output
 * @return bool: whether the parsing was successful or not
 */
bool read_bip32_path(buffer_t *buf, bip32_path_t *const out);

/**
 * @brief Copies a bip32 path
 *
 * @param out: bip32 path output
 * @param in: bip32 path copied
 * @return bool: whether the copy was successful or not
 */
static inline bool copy_bip32_path(bip32_path_t *const out, bip32_path_t const *const in) {
    if ((out == NULL) || (in == NULL)) {
        return false;
    }
    memcpy(out->components, in->components, in->length * sizeof(*in->components));
    out->length = in->length;
    return true;
}

/**
 * @brief Checks that two bip32 paths are equals
 *
 * @param p1: first bip32 path
 * @param p2: second bip32 path
 * @return bool: result
 */
static inline bool bip32_paths_eq(bip32_path_t volatile const *const p1,
                                  bip32_path_t volatile const *const p2) {
    return (p1 == p2) || ((p1 != NULL) && (p2 != NULL) && (p1->length == p2->length) &&
                          (memcmp((const uint8_t *) p1->components,
                                  (const uint8_t *) p2->components,
                                  p1->length * sizeof(*p1->components)) == 0));
}

/**
 * @brief Copies a bip32 path and its curve
 *
 * @param out: output
 * @param in: bip32 path and curve copied
 * @return bool: whether the copy was successful or not
 */
static inline bool copy_bip32_path_with_curve(bip32_path_with_curve_t *const out,
                                              bip32_path_with_curve_t const *const in) {
    if ((out == NULL) || (in == NULL) || !copy_bip32_path(&out->bip32_path, &in->bip32_path)) {
        return false;
    }
    out->derivation_type = in->derivation_type;
    return true;
}

/**
 * @brief Checks that two bip32 paths with their paths are equals
 *
 * @param p1: first bip32 path and curve
 * @param p2: first bip32 path and curve
 * @return bool: result
 */
static inline bool bip32_path_with_curve_eq(bip32_path_with_curve_t volatile const *const p1,
                                            bip32_path_with_curve_t volatile const *const p2) {
    return (p1 == p2) ||
           ((p1 != NULL) && (p2 != NULL) && bip32_paths_eq(&p1->bip32_path, &p2->bip32_path) &&
            (p1->derivation_type == p2->derivation_type));
}

/***** Key *****/

#define KEY_HASH_SIZE     20u
#define PK_LEN            65u
#define COMPRESSED_PK_LEN 33u
#define TZ_EDPK_LEN       (COMPRESSED_PK_LEN - 1u)

/**
 * @brief This structure represents elliptic curve public key handled
 *        Can be explicitly cast into `cx_ecfp_public_key_t`
 */
typedef union {
    cx_ecfp_256_public_key_t pk_256;  ///< edpk, sppk and p2pk keys
#ifndef TARGET_NANOS
    cx_ecfp_384_public_key_t pk_384;  ///< BLpk keys
#endif
} tz_ecfp_public_key_t;

/**
 * @brief Alias to represent abstract compressed public key
 */
typedef cx_ecfp_public_key_t cx_ecfp_compressed_public_key_t;

/**
 * @brief These structures are based on `cx_ecfp_public_key_t` and represents compressed elliptic
 * curve public key handled
 *        Can be explicitly cast into `cx_ecfp_compressed_public_key_t`
 */
/** Ed25519 compressed public key */
typedef struct {
    cx_curve_t curve;        ///< Curve identifier
    size_t W_len;            ///< Compressed public key length in bytes
    uint8_t W[TZ_EDPK_LEN];  ///< Compressed public key value
} tz_ecfp_ed25519_compressed_public_key_t;
/** Secp256 compressed public key */
typedef struct {
    cx_curve_t curve;              ///< Curve identifier
    size_t W_len;                  ///< Compressed public key length in bytes
    uint8_t W[COMPRESSED_PK_LEN];  ///< Compressed public key value
} tz_ecfp_secp256_compressed_public_key_t;
#ifndef TARGET_NANOS
/** BLS compressed public key */
typedef struct {
    cx_curve_t curve;                  ///< Curve identifier
    size_t W_len;                      ///< Compressed public key length in bytes
    uint8_t W[BLS_COMPRESSED_PK_LEN];  ///< Compressed public key value
} tz_ecfp_bls_compressed_public_key_t;
#endif
/**
 * @brief This structure represents elliptic curve compressed public key handled
 */
typedef union {
    tz_ecfp_ed25519_compressed_public_key_t pk_ed25519;  ///< edpk keys
    tz_ecfp_secp256_compressed_public_key_t pk_secp256;  ///< sppk and p2pk keys
#ifndef TARGET_NANOS
    tz_ecfp_bls_compressed_public_key_t pk_bls;  ///< BLpk keys
#endif
} tz_ecfp_compressed_public_key_t;

/**
 * @brief Generates a public key from a bip32 path and a curve
 *
 * @param public_key: public key output
 * @param path_with_curve: bip32 path and curve
 * @return cx_err_t: error, CX_OK if none
 */
cx_err_t generate_public_key(cx_ecfp_public_key_t *public_key,
                             bip32_path_with_curve_t const *const path_with_curve);

/**
 * @brief Extract the public key hash from a public key and a curve
 *
 *        Is non-reentrant
 *
 * @param hash_out: public key hash output
 * @param hash_out_size: output size
 * @param compressed_out: compressed public key output
 *                        pass NULL if this value is not desired
 * @param public_key: public key
 * @return cx_err_t: error, CX_OK if none
 */
cx_err_t public_key_hash(uint8_t *const hash_out,
                         size_t const hash_out_size,
                         cx_ecfp_compressed_public_key_t *compressed_out,
                         cx_ecfp_public_key_t const *const public_key);

/**
 * @brief Signs a message with a key
 *
 *        output_size will be updated to the signature size
 *
 * @param out: signature output
 * @param out_size: output size
 * @param path_with_curve: bip32 path and curve of the key
 * @param public_key: BLS public key associated with the path and the inner seed.
 *                    Will be used only for BLS signature.
 *                    If NULL, it will be computed using the path and the inner seed.
 * @param in: message input
 * @param in_size: input size
 * @return cx_err_t: error, CX_OK if none
 */
cx_err_t sign(uint8_t *const out,
              size_t *out_size,
              bip32_path_with_curve_t const *const path_with_curve,
              cx_ecfp_public_key_t *public_key,
              uint8_t const *const in,
              size_t const in_size);
