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
    // Allocate 64 bytes to respect Syscall API but only 32 will be used
    uint8_t raw_privkey[64] = {0};
    cx_curve_t curve = CX_CURVE_BLS12_381_G1;
    size_t length = 48;

    // Derive private key according to the path
    io_seproxyhal_io_heartbeat();
    CX_CHECK(os_derive_eip2333_no_throw(curve, path, path_len, raw_privkey + 16));
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
                                                        uint8_t raw_pubkey[static 97]) {
    cx_err_t error = CX_OK;
    cx_curve_t curve = CX_CURVE_BLS12_381_G1;

    uint8_t yFlag = 0;
    uint8_t tmp[96] = {0};
    uint8_t bls_field[48] = {0};
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

    tmp[47] = 2;
    CX_CHECK(cx_math_mult_no_throw(tmp, pubkey.W + 1 + 48, tmp, 48));
    CX_CHECK(cx_ecdomain_parameter(curve, CX_CURVE_PARAM_Field, bls_field, sizeof(bls_field)));
    CX_CHECK(cx_math_cmp_no_throw(tmp + 48, bls_field, 48, &diff));
    if (diff > 0) {
        yFlag = 0x20u;
    }
    pubkey.W[1] &= 0x1fu;
    pubkey.W[1] |= 0x80u | yFlag;

    // Check pubkey length then copy it to raw_pubkey
    if (pubkey.W_len != 97) {
        error = CX_EC_INVALID_CURVE;
        goto end;
    }
    memmove(raw_pubkey, pubkey.W, pubkey.W_len);

end:
    explicit_bzero(&privkey, sizeof(privkey));

    if (error != CX_OK) {
        // Make sure the caller doesn't use uninitialized data in case
        // the return code is not checked.
        explicit_bzero(raw_pubkey, 97);
    }
    return error;
}

#ifndef TARGET_NANOS
WARN_UNUSED_RESULT cx_err_t bip32_derive_with_seed_bls_sign_hash(const uint32_t *path,
                                                                 size_t path_len,
                                                                 uint8_t const *msg,
                                                                 uint8_t msg_len,
                                                                 uint8_t *sig,
                                                                 size_t *sig_len) {
    cx_err_t error = CX_OK;
    // cipher suite used in tezos:
    // https://gitlab.com/tezos/tezos/-/blob/master/src/lib_bls12_381_signature/bls12_381_signature.ml?ref_type=heads#L351
    const uint8_t ciphersuite[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_AUG_";
    cx_ecfp_384_private_key_t privkey;
    uint8_t hash[192];

    if ((sig_len == NULL) || (*sig_len < 96)) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    // Derive private key according to BIP32 path
    CX_CHECK(bip32_derive_init_privkey_bls(path, path_len, &privkey));

    CX_CHECK(
        cx_hash_to_field(msg, msg_len, ciphersuite, sizeof(ciphersuite) - 1u, hash, sizeof(hash)));

    CX_CHECK(ox_bls12381_sign(&privkey, hash, sizeof(hash), sig, 96));
    *sig_len = 96;

end:
    explicit_bzero(&privkey, sizeof(privkey));

    if (error != CX_OK) {
        // Make sure the caller doesn't use uninitialized data in case
        // the return code is not checked.
        explicit_bzero(sig, 96);
    }
    return error;
}
#endif
