/* Tezos Ledger application - HMAC handler

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

#include "apdu_hmac.h"

#include "globals.h"
#include "keys.h"
#include "protocol.h"

#define G global.apdu.u.hmac

/**
 * @brief Generate the hmac of a message
 *
 *        The hmac-key is the signature of a fixed message signed with a given key
 *
 * @param out: result output
 * @param out_size: output size
 * @param state: hmac state
 * @param in: input message
 * @param in_size: input size
 * @param bip32_path: key path
 * @param derivation_type: key curve
 * @return size_t: size of the hmac
 */
static inline size_t hmac(uint8_t *const out,
                          size_t const out_size,
                          apdu_hmac_state_t *const state,
                          uint8_t const *const in,
                          size_t const in_size,
                          bip32_path_t bip32_path,
                          derivation_type_t derivation_type) {
    check_null(out);
    check_null(state);
    check_null(in);
    if (out_size < CX_SHA256_SIZE) {
        THROW(EXC_WRONG_LENGTH);
    }

    // Pick a static, arbitrary SHA256 value based on a quote of Jesus.
    static uint8_t const key_sha256[] = {0x6c, 0x4e, 0x7e, 0x70, 0x6c, 0x54, 0xd3, 0x67,
                                         0xc8, 0x7a, 0x8d, 0x89, 0xc1, 0x6a, 0xdf, 0xe0,
                                         0x6c, 0xb5, 0x68, 0x0c, 0xb7, 0xd1, 0x8e, 0x62,
                                         0x5a, 0x90, 0x47, 0x5e, 0xc0, 0xdb, 0xdb, 0x9f};

    // Deterministically sign the SHA256 value to get something directly tied to the secret key.
    key_pair_t key_pair = {0};

    size_t signed_hmac_key_size = 0;

    BEGIN_TRY {
        TRY {
            generate_key_pair(&key_pair, derivation_type, &bip32_path);
            signed_hmac_key_size = sign(state->signed_hmac_key,
                                        sizeof(state->signed_hmac_key),
                                        derivation_type,
                                        &key_pair,
                                        key_sha256,
                                        sizeof(key_sha256));
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            memset(&key_pair, 0, sizeof(key_pair));
        }
    }
    END_TRY;

    // Hash the signed value with SHA512 to get a 64-byte key for HMAC.
    cx_hash_sha512(state->signed_hmac_key,
                   signed_hmac_key_size,
                   state->hashed_signed_hmac_key,
                   sizeof(state->hashed_signed_hmac_key));

    return cx_hmac_sha256(state->hashed_signed_hmac_key,
                          sizeof(state->hashed_signed_hmac_key),
                          in,
                          in_size,
                          out,
                          out_size);
}

size_t handle_apdu_hmac(__attribute__((unused)) uint8_t instruction,
                        __attribute__((unused)) volatile uint32_t *flags) {
    if (G_io_apdu_buffer[OFFSET_P1] != 0) {
        THROW(EXC_WRONG_PARAM);
    }

    uint8_t const *const buff = &G_io_apdu_buffer[OFFSET_CDATA];
    uint8_t const buff_size = G_io_apdu_buffer[OFFSET_LC];
    if (buff_size > MAX_APDU_SIZE) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }

    memset(&G, 0, sizeof(G));

    derivation_type_t derivation_type = parse_derivation_type(G_io_apdu_buffer[OFFSET_CURVE]);

    bip32_path_t bip32_path = {0};
    size_t consumed = 0;
    consumed += read_bip32_path(&bip32_path, buff, buff_size);

    uint8_t const *const data_to_hmac = &buff[consumed];
    size_t const data_to_hmac_size = buff_size - consumed;

    size_t const hmac_size = hmac(G.hmac,
                                  sizeof(G.hmac),
                                  &G,
                                  data_to_hmac,
                                  data_to_hmac_size,
                                  bip32_path,
                                  derivation_type);

    size_t tx = 0;
    memcpy(G_io_apdu_buffer, G.hmac, hmac_size);
    tx += hmac_size;
    return finalize_successful_send(tx);
}
