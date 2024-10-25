/* Tezos Ledger application - Cryptography operations

   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger SAS

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
#ifndef TARGET_NANOS

#include <stdint.h>  // uint*_t

#include "cx.h"

#define BLS_SK_LEN            48u
#define BLS_PK_LEN            (1u + (BLS_SK_LEN * 2u))
#define BLS_COMPRESSED_PK_LEN 48u
#define BLS_SIG_LEN           96u

// `BLS_MAX_MESSAGE_LEN` can be as large as needed
#define BLS_MAX_MESSAGE_LEN 235u

/**
 * @brief   Gets the bls public key from the device seed using the specified bip32 path
 * key.
 *
 * @param[in]  path            Bip32 path to use for derivation.
 *
 * @param[in]  path_len        Bip32 path length.
 *
 * @param[out] raw_pubkey      Buffer where to store the public key.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_EC_INVALID_CURVE
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT cx_err_t bip32_derive_get_pubkey_bls(const uint32_t *path,
                                                        size_t path_len,
                                                        uint8_t raw_pubkey[static 97]);

/**
 * @brief   Sign a hash with bls using the device seed derived from the specified bip32 path.
 *
 * @param[in]  path            Bip32 path to use for derivation.
 *
 * @param[in]  path_len        Bip32 path length.
 *
 * @param[in]  public_key      Must be the BLS key associated with the path and the inner seed.
 *                             If NULL, it will be computed using the path and the inner seed.
 *
 * @param[in]  msg             Digest of the message to be signed.
 *                             The length of *message* must be shorter than the group order size.
 *                             Otherwise it is truncated.
 *
 * @param[in]  msg_len         Length of the digest in octets.
 *
 * @param[out] sig             Buffer where to store the signature.
 *
 * @param[in]  sig_len         Length of the signature buffer, updated with signature length.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_EC_INVALID_CURVE
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT cx_err_t
bip32_derive_with_seed_bls_sign_hash(const uint32_t *path,
                                     size_t path_len,
                                     cx_ecfp_384_public_key_t *public_key,
                                     uint8_t const *msg,
                                     size_t msg_len,
                                     uint8_t *sig,
                                     size_t *sig_len);
#endif
