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
    size_t signed_hmac_key_size = sign(state->signed_hmac_key,
                                       sizeof(state->signed_hmac_key),
                                       derivation_type,
                                       &bip32_path,
                                       key_sha256,
                                       sizeof(key_sha256));

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

/**
 * Cdata:
 *   + Bip32 path: signing key path
 *   + (max-size) uint8 *: message
 */
int handle_hmac(buffer_t *cdata, derivation_type_t derivation_type) {
    check_null(cdata);

    memset(&G, 0, sizeof(G));

    bip32_path_t bip32_path = {0};

    if (!read_bip32_path(cdata, &bip32_path)) {
        THROW(EXC_WRONG_VALUES);
    }

    size_t const hmac_size = hmac(G.hmac,
                                  sizeof(G.hmac),
                                  &G,
                                  cdata->ptr + cdata->offset,
                                  cdata->size - cdata->offset,
                                  bip32_path,
                                  derivation_type);

    uint8_t resp[CX_SHA256_SIZE] = {0};

    memcpy(resp, G.hmac, hmac_size);

    return io_send_response_pointer(resp, hmac_size, SW_OK);
}
