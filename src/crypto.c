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

#include <stdint.h>   // uint*_t
#include <string.h>   // memset, explicit_bzero
#include <stdbool.h>  // bool

#include "crypto.h"

#include "cx.h"
#include "os.h"
#include "os_io_seproxyhal.h"

#ifndef TARGET_NANOS

/**
 * @brief   Gets the bls private key from the device seed using the specified bip32 path
 * key.
 *
 * @param[in]  path            Bip32 path to use for derivation.
 *
 * @param[in]  path_len        Bip32 path length.
 *
 * @param[out] privkey         Generated private key.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_EC_INVALID_CURVE
 *                             - CX_INTERNAL_ERROR
 */
static WARN_UNUSED_RESULT cx_err_t
bip32_derive_init_privkey_bls(const uint32_t *path,
                              size_t path_len,
                              cx_ecfp_384_private_key_t *privkey) {
    cx_err_t error = CX_OK;
    // Allocate 64 bytes to respect Syscall API but only 48 will be used
    uint8_t raw_privkey[64] = {0};
    cx_curve_t curve = CX_CURVE_BLS12_381_G1;
    size_t length = BLS_SK_LEN;

    // Derive private key according to the path
    io_seproxyhal_io_heartbeat();
    CX_CHECK(os_derive_eip2333_no_throw(curve, path, path_len, raw_privkey + (64u - BLS_SK_LEN)));
    io_seproxyhal_io_heartbeat();

    // Init privkey from raw
    CX_CHECK(cx_ecfp_init_private_key_no_throw(curve,
                                               raw_privkey,
                                               length,
                                               (cx_ecfp_private_key_t *) privkey));

end:
    explicit_bzero(raw_privkey, sizeof(raw_privkey));

    if (error != CX_OK) {
        // Make sure the caller doesn't use uninitialized data in case
        // the return code is not checked.
        explicit_bzero(privkey, sizeof(cx_ecfp_384_private_key_t));
    }
    return error;
}

WARN_UNUSED_RESULT cx_err_t bip32_derive_get_pubkey_bls(const uint32_t *path,
                                                        size_t path_len,
                                                        uint8_t raw_pubkey[static BLS_PK_LEN]) {
    cx_err_t error = CX_OK;
    cx_curve_t curve = CX_CURVE_BLS12_381_G1;

    uint8_t tmp[BLS_SK_LEN * 2u] = {0};
    uint8_t bls_field[BLS_SK_LEN] = {0};
    int diff = 0;

    cx_ecfp_384_private_key_t privkey = {0};
    cx_ecfp_384_public_key_t pubkey = {0};

    // Derive private key according to BIP32 path
    CX_CHECK(bip32_derive_init_privkey_bls(path, path_len, &privkey));

    // Generate associated pubkey
    CX_CHECK(cx_ecfp_generate_pair_no_throw(curve,
                                            (cx_ecfp_public_key_t *) &pubkey,
                                            (cx_ecfp_private_key_t *) &privkey,
                                            true));

    tmp[BLS_SK_LEN - 1u] = 2;
    CX_CHECK(cx_math_mult_no_throw(tmp, pubkey.W + 1u + BLS_SK_LEN, tmp, BLS_SK_LEN));
    CX_CHECK(cx_ecdomain_parameter(curve, CX_CURVE_PARAM_Field, bls_field, sizeof(bls_field)));
    CX_CHECK(cx_math_cmp_no_throw(tmp + BLS_SK_LEN, bls_field, BLS_SK_LEN, &diff));
    pubkey.W[1] &= 0x1fu;
    if (diff > 0) {
        pubkey.W[1] |= 0xa0u;
    } else {
        pubkey.W[1] |= 0x80u;
    }

    // Check pubkey length then copy it to raw_pubkey
    if (pubkey.W_len != BLS_PK_LEN) {
        error = CX_EC_INVALID_CURVE;
        goto end;
    }
    memmove(raw_pubkey, pubkey.W, pubkey.W_len);

end:
    explicit_bzero(&privkey, sizeof(privkey));

    if (error != CX_OK) {
        // Make sure the caller doesn't use uninitialized data in case
        // the return code is not checked.
        explicit_bzero(raw_pubkey, BLS_PK_LEN);
    }
    return error;
}

// Cipher suite used in tezos:
// https://gitlab.com/tezos/tezos/-/blob/master/src/lib_bls12_381_signature/bls12_381_signature.ml?ref_type=heads#L351
static const uint8_t CIPHERSUITE[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_AUG_";

WARN_UNUSED_RESULT cx_err_t
bip32_derive_with_seed_bls_sign_hash(const uint32_t *path,
                                     size_t path_len,
                                     cx_ecfp_384_public_key_t *public_key,
                                     uint8_t const *msg,
                                     size_t msg_len,
                                     uint8_t *sig,
                                     size_t *sig_len) {
    cx_err_t error = CX_OK;
    cx_ecfp_384_private_key_t privkey = {0};
    uint8_t hash[CX_BLS_BLS12381_PARAM_LEN * 4] = {0};
    uint8_t tmp[BLS_COMPRESSED_PK_LEN + BLS_MAX_MESSAGE_LEN] = {0};
    uint8_t raw_pubkey[BLS_PK_LEN] = {0};

    if ((sig_len == NULL) || (*sig_len < BLS_SIG_LEN)) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    if (BLS_MAX_MESSAGE_LEN < msg_len) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    // Derive private key according to BIP32 path
    CX_CHECK(bip32_derive_init_privkey_bls(path, path_len, &privkey));

    if (public_key == NULL) {
        CX_CHECK(bip32_derive_get_pubkey_bls(path, path_len, raw_pubkey));
        memmove(tmp, raw_pubkey + 1, BLS_COMPRESSED_PK_LEN);
    } else {
        if ((public_key->curve != CX_CURVE_BLS12_381_G1) ||
            (public_key->W_len < (BLS_COMPRESSED_PK_LEN + 1u))) {
            error = CX_INVALID_PARAMETER_VALUE;
            goto end;
        }
        memmove(tmp, public_key->W + 1, BLS_COMPRESSED_PK_LEN);
    }
    memmove(tmp + BLS_COMPRESSED_PK_LEN, msg, msg_len);

    CX_CHECK(cx_hash_to_field(tmp,
                              BLS_COMPRESSED_PK_LEN + msg_len,
                              CIPHERSUITE,
                              sizeof(CIPHERSUITE) - 1u,
                              hash,
                              sizeof(hash)));

    CX_CHECK(ox_bls12381_sign(&privkey, hash, sizeof(hash), sig, BLS_SIG_LEN));
    *sig_len = BLS_SIG_LEN;

end:
    explicit_bzero(&privkey, sizeof(privkey));
    explicit_bzero(hash, sizeof(hash));
    explicit_bzero(tmp, sizeof(tmp));
    explicit_bzero(raw_pubkey, sizeof(raw_pubkey));

    if (error != CX_OK) {
        // Make sure the caller doesn't use uninitialized data in case
        // the return code is not checked.
        explicit_bzero(sig, BLS_SIG_LEN);
    }
    return error;
}
#endif
