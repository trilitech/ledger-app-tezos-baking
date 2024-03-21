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

#include "keys.h"

#include "apdu.h"
#include "crypto_helpers.h"
#include "globals.h"
#include "memory.h"
#include "types.h"

#include <stdbool.h>
#include <string.h>

bool read_bip32_path(buffer_t *buf, bip32_path_t *const out) {
    return (buffer_read_u8(buf, &out->length) &&
            buffer_read_bip32_path(buf, out->components, (size_t) out->length));
}

/**
 * @brief Converts `derivation_type` to `derivation_mode`
 *
 * @param derivation_type: derivation_type
 * @return unsigned int: derivation mode
 */
static inline unsigned int derivation_type_to_derivation_mode(
    derivation_type_t const derivation_type) {
    switch (derivation_type) {
        case DERIVATION_TYPE_ED25519:
            return HDW_ED25519_SLIP10;
        case DERIVATION_TYPE_SECP256K1:
        case DERIVATION_TYPE_SECP256R1:
        case DERIVATION_TYPE_BIP32_ED25519:
        default:
            return HDW_NORMAL;
    }
}

/**
 * @brief Converts `signature_type` to `cx_curve`
 *
 * @param signature_type: signature_type
 * @return cx_curve_t: curve result
 */
static inline cx_curve_t signature_type_to_cx_curve(signature_type_t const signature_type) {
    switch (signature_type) {
        case SIGNATURE_TYPE_SECP256K1:
            return CX_CURVE_SECP256K1;
        case SIGNATURE_TYPE_SECP256R1:
            return CX_CURVE_SECP256R1;
        case SIGNATURE_TYPE_ED25519:
            return CX_CURVE_Ed25519;
        default:
            return CX_CURVE_NONE;
    }
}

int generate_public_key(cx_ecfp_public_key_t *public_key,
                        bip32_path_with_curve_t const *const path_with_curve) {
    int error = 0;

    bip32_path_t const *const bip32_path = &path_with_curve->bip32_path;
    derivation_type_t derivation_type = path_with_curve->derivation_type;
    unsigned int derivation_mode = derivation_type_to_derivation_mode(derivation_type);
    signature_type_t signature_type = derivation_type_to_signature_type(derivation_type);
    cx_curve_t cx_curve = signature_type_to_cx_curve(signature_type);

    public_key->W_len = 65;
    public_key->curve = cx_curve;

    // generate corresponding public key
    error = bip32_derive_with_seed_get_pubkey_256(derivation_mode,
                                                  public_key->curve,
                                                  bip32_path->components,
                                                  bip32_path->length,
                                                  public_key->W,
                                                  NULL,
                                                  CX_SHA512,
                                                  NULL,
                                                  0);
    if (error != 0) {
        return error;
    }

    // If we're using the old curve, make sure to adjust accordingly.
    if (cx_curve == CX_CURVE_Ed25519) {
        error =
            cx_edwards_compress_point_no_throw(CX_CURVE_Ed25519, public_key->W, public_key->W_len);
        public_key->W_len = 33;
    }

    return error;
}

void public_key_hash(uint8_t *const hash_out,
                     size_t const hash_out_size,
                     cx_ecfp_public_key_t *compressed_out,
                     derivation_type_t const derivation_type,
                     cx_ecfp_public_key_t const *const public_key) {
    check_null(hash_out);
    check_null(public_key);
    if (hash_out_size < HASH_SIZE) {
        THROW(EXC_WRONG_LENGTH);
    }

    signature_type_t signature_type = derivation_type_to_signature_type(derivation_type);
    cx_ecfp_public_key_t compressed = {0};

    switch (signature_type) {
        case SIGNATURE_TYPE_ED25519: {
            compressed.W_len = public_key->W_len - 1;
            memcpy(compressed.W, public_key->W + 1, compressed.W_len);
            break;
        }
        case SIGNATURE_TYPE_SECP256K1:
        case SIGNATURE_TYPE_SECP256R1: {
            memcpy(compressed.W, public_key->W, public_key->W_len);
            compressed.W[0] = 0x02 + (public_key->W[64] & 0x01);
            compressed.W_len = 33;
            break;
        }
        default:
            THROW(EXC_WRONG_PARAM);
    }

    cx_blake2b_t hash_state;
    // cx_blake2b_init takes size in bits.
    CX_THROW(cx_blake2b_init_no_throw(&hash_state, HASH_SIZE * 8u));
    CX_THROW(cx_hash_no_throw((cx_hash_t *) &hash_state,
                              CX_LAST,
                              compressed.W,
                              compressed.W_len,
                              hash_out,
                              HASH_SIZE));
    if (compressed_out != NULL) {
        memmove(compressed_out, &compressed, sizeof(*compressed_out));
    }
}

size_t sign(uint8_t *const out,
            size_t const out_size,
            bip32_path_with_curve_t const *const path_with_curve,
            uint8_t const *const in,
            size_t const in_size) {
    check_null(out);
    check_null(path_with_curve);
    check_null(in);

    bip32_path_t const *const bip32_path = &path_with_curve->bip32_path;
    derivation_type_t derivation_type = path_with_curve->derivation_type;
    unsigned int derivation_mode = derivation_type_to_derivation_mode(derivation_type);
    signature_type_t signature_type = derivation_type_to_signature_type(derivation_type);
    cx_curve_t cx_curve = signature_type_to_cx_curve(signature_type);

    size_t tx = 0;
    switch (signature_type) {
        case SIGNATURE_TYPE_ED25519: {
            static size_t const SIG_SIZE = 64u;
            if (out_size < SIG_SIZE) {
                THROW(EXC_WRONG_LENGTH);
            }
            size_t sig_len = SIG_SIZE;
            CX_THROW(bip32_derive_with_seed_eddsa_sign_hash_256(derivation_mode,
                                                                cx_curve,
                                                                bip32_path->components,
                                                                bip32_path->length,
                                                                CX_SHA512,
                                                                (uint8_t const *) PIC(in),
                                                                in_size,
                                                                out,
                                                                &sig_len,
                                                                NULL,
                                                                0));

            tx += SIG_SIZE;

        } break;
        case SIGNATURE_TYPE_SECP256K1:
        case SIGNATURE_TYPE_SECP256R1: {
            static size_t const SIG_SIZE = 100u;
            if (out_size < SIG_SIZE) {
                THROW(EXC_WRONG_LENGTH);
            }
            unsigned int info;
            size_t sig_len = SIG_SIZE;
            CX_THROW(bip32_derive_ecdsa_sign_hash_256(cx_curve,
                                                      bip32_path->components,
                                                      bip32_path->length,
                                                      CX_LAST | CX_RND_RFC6979,
                                                      CX_SHA256,
                                                      (uint8_t const *) PIC(in),
                                                      in_size,
                                                      out,
                                                      &sig_len,
                                                      &info));
            tx += sig_len;

            if ((info & CX_ECCINFO_PARITY_ODD) != 0) {
                out[0] |= 0x01;
            }
        } break;
        default:
            THROW(EXC_WRONG_PARAM);  // This should not be able to happen.
    }

    return tx;
}
