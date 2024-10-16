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

#include "crypto_helpers.h"

#include "keys.h"

/***** Bip32 path *****/

bool read_bip32_path(buffer_t *buf, bip32_path_t *const out) {
    return (buffer_read_u8(buf, &out->length) &&
            buffer_read_bip32_path(buf, out->components, (size_t) out->length));
}

/***** Key *****/

cx_err_t generate_public_key(cx_ecfp_public_key_t *public_key,
                             bip32_path_with_curve_t const *const path_with_curve) {
    if ((public_key == NULL) || (path_with_curve == NULL)) {
        return CX_INVALID_PARAMETER;
    }

    cx_err_t error = CX_OK;

    bip32_path_t const *const bip32_path = &path_with_curve->bip32_path;
    derivation_type_t derivation_type = path_with_curve->derivation_type;
    unsigned int derivation_mode;

    switch (derivation_type) {
        case DERIVATION_TYPE_ED25519:
        case DERIVATION_TYPE_BIP32_ED25519: {
            derivation_mode =
                (derivation_type == DERIVATION_TYPE_ED25519) ? HDW_ED25519_SLIP10 : HDW_NORMAL;
            public_key->curve = CX_CURVE_Ed25519;
            public_key->W_len = COMPRESSED_PK_LEN;
            CX_CHECK(
                bip32_derive_with_seed_get_pubkey_256(derivation_mode,
                                                      public_key->curve,
                                                      bip32_path->components,
                                                      bip32_path->length,
                                                      ((cx_ecfp_256_public_key_t *) public_key)->W,
                                                      NULL,
                                                      CX_SHA512,
                                                      NULL,
                                                      0));
            CX_CHECK(cx_edwards_compress_point_no_throw(CX_CURVE_Ed25519,
                                                        public_key->W,
                                                        public_key->W_len));
            break;
        }
        case DERIVATION_TYPE_SECP256K1:
        case DERIVATION_TYPE_SECP256R1: {
            public_key->curve = (derivation_type == DERIVATION_TYPE_SECP256K1) ? CX_CURVE_SECP256K1
                                                                               : CX_CURVE_SECP256R1;
            public_key->W_len = PK_LEN;
            CX_CHECK(bip32_derive_get_pubkey_256(public_key->curve,
                                                 bip32_path->components,
                                                 bip32_path->length,
                                                 ((cx_ecfp_256_public_key_t *) public_key)->W,
                                                 NULL,
                                                 CX_SHA512));
            break;
        }
        default:
            return CX_INVALID_PARAMETER;
    }

end:
    return error;
}

/**
 * @brief Extract the public key hash from a public key and a curve
 *
 *        Is non-reentrant
 *
 * @param hash_out: public key hash output
 * @param hash_out_size: output size
 * @param compressed_out: compressed public key output
 *                        pass NULL if this value is not desired
 * @param derivation_type: curve
 * @param param public_key: public key
 * @return cx_err_t: error, CX_OK if none
 */
static cx_err_t public_key_hash(uint8_t *const hash_out,
                                size_t const hash_out_size,
                                cx_ecfp_compressed_public_key_t *compressed_out,
                                derivation_type_t const derivation_type,
                                cx_ecfp_public_key_t const *const public_key) {
    if ((hash_out == NULL) || (public_key == NULL)) {
        return CX_INVALID_PARAMETER;
    }

    if (hash_out_size < KEY_HASH_SIZE) {
        return CX_INVALID_PARAMETER_SIZE;
    }

    cx_ecfp_compressed_public_key_t *compressed =
        (cx_ecfp_compressed_public_key_t *) &(tz_ecfp_compressed_public_key_t){0};

    switch (derivation_type) {
        case DERIVATION_TYPE_ED25519:
        case DERIVATION_TYPE_BIP32_ED25519: {
            compressed->curve = public_key->curve;
            compressed->W_len = TZ_EDPK_LEN;
            memcpy(compressed->W, public_key->W + 1, compressed->W_len);
            break;
        }
        case DERIVATION_TYPE_SECP256K1:
        case DERIVATION_TYPE_SECP256R1: {
            compressed->curve = public_key->curve;
            compressed->W_len = COMPRESSED_PK_LEN;
            memcpy(compressed->W, public_key->W, compressed->W_len);
            compressed->W[0] = 0x02 + (public_key->W[64] & 0x01);
            break;
        }
        default:
            return CX_INVALID_PARAMETER;
    }

    cx_err_t error = CX_OK;
    cx_blake2b_t hash_state;
    // cx_blake2b_init takes size in bits.
    CX_CHECK(cx_blake2b_init_no_throw(&hash_state, KEY_HASH_SIZE * 8u));

    CX_CHECK(cx_hash_no_throw((cx_hash_t *) &hash_state,
                              CX_LAST,
                              compressed->W,
                              compressed->W_len,
                              hash_out,
                              KEY_HASH_SIZE));

    if (compressed_out != NULL) {
        memmove(compressed_out, compressed, sizeof(tz_ecfp_compressed_public_key_t));
    }

end:
    return error;
}

cx_err_t generate_public_key_hash(uint8_t *const hash_out,
                                  size_t const hash_out_size,
                                  cx_ecfp_compressed_public_key_t *compressed_out,
                                  bip32_path_with_curve_t const *const path_with_curve) {
    if ((hash_out == NULL) || (path_with_curve == NULL)) {
        return CX_INVALID_PARAMETER;
    }

    cx_ecfp_public_key_t *pubkey = (cx_ecfp_public_key_t *) &(tz_ecfp_public_key_t){0};
    cx_err_t error = CX_OK;

    CX_CHECK(generate_public_key(pubkey, path_with_curve));

    CX_CHECK(public_key_hash(hash_out,
                             hash_out_size,
                             compressed_out,
                             path_with_curve->derivation_type,
                             pubkey));

end:
    return error;
}

cx_err_t sign(uint8_t *const out,
              size_t *out_size,
              bip32_path_with_curve_t const *const path_with_curve,
              uint8_t const *const in,
              size_t const in_size) {
    if ((out == NULL) || (out_size == NULL) || (path_with_curve == NULL) || (in == NULL)) {
        return CX_INVALID_PARAMETER;
    }

    cx_err_t error = CX_OK;

    bip32_path_t const *const bip32_path = &path_with_curve->bip32_path;
    derivation_type_t derivation_type = path_with_curve->derivation_type;
    unsigned int derivation_mode;
    cx_curve_t cx_curve;
    uint32_t info;

    switch (derivation_type) {
        case DERIVATION_TYPE_ED25519:
        case DERIVATION_TYPE_BIP32_ED25519: {
            derivation_mode =
                (derivation_type == DERIVATION_TYPE_ED25519) ? HDW_ED25519_SLIP10 : HDW_NORMAL;
            cx_curve = CX_CURVE_Ed25519;
            CX_CHECK(bip32_derive_with_seed_eddsa_sign_hash_256(derivation_mode,
                                                                cx_curve,
                                                                bip32_path->components,
                                                                bip32_path->length,
                                                                CX_SHA512,
                                                                (uint8_t const *) PIC(in),
                                                                in_size,
                                                                out,
                                                                out_size,
                                                                NULL,
                                                                0));
        } break;
        case DERIVATION_TYPE_SECP256K1:
        case DERIVATION_TYPE_SECP256R1: {
            cx_curve = (derivation_type == DERIVATION_TYPE_SECP256K1) ? CX_CURVE_SECP256K1
                                                                      : CX_CURVE_SECP256R1;
            CX_CHECK(bip32_derive_ecdsa_sign_hash_256(cx_curve,
                                                      bip32_path->components,
                                                      bip32_path->length,
                                                      CX_LAST | CX_RND_RFC6979,
                                                      CX_SHA256,
                                                      (uint8_t const *) PIC(in),
                                                      in_size,
                                                      out,
                                                      out_size,
                                                      &info));
            if ((info & CX_ECCINFO_PARITY_ODD) != 0) {
                out[0] |= 0x01;
            }
        } break;
        default:
            error = CX_INVALID_PARAMETER;
    }

end:
    return error;
}
