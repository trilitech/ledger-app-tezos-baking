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
#include "globals.h"
#include "memory.h"
#include "protocol.h"
#include "types.h"

#include <stdbool.h>
#include <string.h>

size_t read_bip32_path(bip32_path_t *const out, uint8_t const *const in, size_t const in_size) {
    struct bip32_path_wire const *const buf_as_bip32 = (struct bip32_path_wire const *) in;

    if (in_size < sizeof(buf_as_bip32->length)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    size_t ix = 0;
    out->length = CONSUME_UNALIGNED_BIG_ENDIAN(ix, uint8_t, &buf_as_bip32->length);

    if (in_size - ix < out->length * sizeof(*buf_as_bip32->components)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }
    if (out->length == 0 || out->length > NUM_ELEMENTS(out->components)) {
        THROW(EXC_WRONG_VALUES);
    }

    for (size_t j = 0; j < out->length; j++) {
        out->components[j] =
            CONSUME_UNALIGNED_BIG_ENDIAN(ix, uint32_t, &buf_as_bip32->components[j]);
    }

    return ix;
}

/**
 * @brief Derivates private key from bip32 path and curve
 *
 * @param private_key: private key output
 * @param derivation_type: curve
 * @param bip32_path: bip32 path
 * @return int: error, 0 if none
 */
static int crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                                     derivation_type_t const derivation_type,
                                     bip32_path_t const *const bip32_path) {
    check_null(bip32_path);
    uint8_t raw_private_key[PRIVATE_KEY_DATA_SIZE] = {0};
    int error = 0;

    cx_curve_t const cx_curve =
        signature_type_to_cx_curve(derivation_type_to_signature_type(derivation_type));

    if (derivation_type == DERIVATION_TYPE_ED25519) {
        // Old, non BIP32_Ed25519 way...
        error = os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10,
                                                   CX_CURVE_Ed25519,
                                                   bip32_path->components,
                                                   bip32_path->length,
                                                   raw_private_key,
                                                   NULL,
                                                   NULL,
                                                   0);
    } else {
        // derive the seed with bip32_path
        error = os_derive_bip32_no_throw(cx_curve,
                                         bip32_path->components,
                                         bip32_path->length,
                                         raw_private_key,
                                         NULL);
    }

    if (!error) {
        // new private_key from raw
        error = cx_ecfp_init_private_key_no_throw(cx_curve, raw_private_key, 32, private_key);
    }

    explicit_bzero(raw_private_key, sizeof(raw_private_key));

    return error;
}

/**
 * @brief Derivates public key from private and curve
 *
 * @param derivation_type: curve
 * @param private_key: private key
 * @param public_key public key output
 * @return int: error, 0 if none
 */
static int crypto_init_public_key(derivation_type_t const derivation_type,
                                  cx_ecfp_private_key_t *private_key,
                                  cx_ecfp_public_key_t *public_key) {
    int error = 0;
    cx_curve_t const cx_curve =
        signature_type_to_cx_curve(derivation_type_to_signature_type(derivation_type));

    // generate corresponding public key
    error = cx_ecfp_generate_pair_no_throw(cx_curve, public_key, private_key, 1);
    if (error) {
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

// The caller should not forget to bzero out the `key_pair` as it contains sensitive information.
int generate_key_pair(key_pair_t *key_pair,
                      derivation_type_t const derivation_type,
                      bip32_path_t const *const bip32_path) {
    int error;

    // derive private key according to BIP32 path
    error = crypto_derive_private_key(&key_pair->private_key, derivation_type, bip32_path);
    if (error) {
        return error;
    }
    // generate corresponding public key
    error = crypto_init_public_key(derivation_type, &key_pair->private_key, &key_pair->public_key);
    return error;
}

int generate_public_key(cx_ecfp_public_key_t *public_key,
                        derivation_type_t const derivation_type,
                        bip32_path_t const *const bip32_path) {
    cx_ecfp_private_key_t private_key = {0};
    int error;

    error = crypto_derive_private_key(&private_key, derivation_type, bip32_path);
    if (error) {
        return (error);
    }
    error = crypto_init_public_key(derivation_type, &private_key, public_key);
    return (error);
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

    cx_ecfp_public_key_t compressed = {0};
    switch (derivation_type_to_signature_type(derivation_type)) {
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
    CX_THROW(cx_blake2b_init_no_throw(&hash_state, HASH_SIZE * 8));
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
            derivation_type_t const derivation_type,
            key_pair_t const *const pair,
            uint8_t const *const in,
            size_t const in_size) {
    check_null(out);
    check_null(pair);
    check_null(in);

    size_t tx = 0;
    switch (derivation_type_to_signature_type(derivation_type)) {
        case SIGNATURE_TYPE_ED25519: {
            static size_t const SIG_SIZE = 64;
            if (out_size < SIG_SIZE) {
                THROW(EXC_WRONG_LENGTH);
            }

            CX_THROW(cx_eddsa_sign_no_throw(&pair->private_key,
                                            CX_SHA512,
                                            (uint8_t const *) PIC(in),
                                            in_size,
                                            out,
                                            SIG_SIZE));

            tx += SIG_SIZE;

        } break;
        case SIGNATURE_TYPE_SECP256K1:
        case SIGNATURE_TYPE_SECP256R1: {
            static size_t const SIG_SIZE = 100;
            if (out_size < SIG_SIZE) {
                THROW(EXC_WRONG_LENGTH);
            }
            unsigned int info;
            size_t sig_len = SIG_SIZE;
            CX_THROW(cx_ecdsa_sign_no_throw(&pair->private_key,
                                            CX_LAST | CX_RND_RFC6979,
                                            CX_SHA256,  // historical reasons...semantically CX_NONE
                                            (uint8_t const *) PIC(in),
                                            in_size,
                                            out,
                                            &sig_len,
                                            &info));
            tx += sig_len;

            if (info & CX_ECCINFO_PARITY_ODD) {
                out[0] |= 0x01;
            }
        } break;
        default:
            THROW(EXC_WRONG_PARAM);  // This should not be able to happen.
    }

    return tx;
}
