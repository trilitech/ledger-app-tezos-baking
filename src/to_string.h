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

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "keys.h"
#include "operations.h"
#include "os_cx.h"
#include "types.h"
#include "ui.h"

#define TICKER_WITH_SPACE " XTZ"

/**
 * @brief Converts a key to a public key hash string using its public key
 *
 * @param out: result output
 * @param out_size: output size
 * @param public_key: public key
 * @return tz_exc: exception, SW_OK if none
 */
tz_exc pk_to_pkh_string(char *const out,
                        size_t const out_size,
                        cx_ecfp_public_key_t const *const public_key);

/**
 * @brief Converts a chain id to string
 *
 *        Outputs its alias if it has one
 *
 * @param dest: result output
 * @param dest_size: output size
 * @param chain_id: chain id to convert
 * @return int: size of the result, negative integer on failure
 */
int chain_id_to_string_with_aliases(char *const dest,
                                    size_t const dest_size,
                                    chain_id_t const *const chain_id);

/**
 * @brief Converts an uint64 number to string
 *
 * @param dest: result output
 * @param dest_size: size of the output
 * @param number: number to convert
 * @return int: size of the result, negative integer on failure
 */
int number_to_string(char *const dest, size_t dest_size, uint64_t number);

/**
 * @brief Converts an uint64 number to microtez as string
 *
 * @param dest: result output
 * @param dest_size: size of the output
 * @param number: number to convert
 * @return int: size of the result, negative integer on failure
 */
int microtez_to_string(char *const dest, size_t dest_size, uint64_t number);

/**
 * @brief Converts a high watermark to string
 *
 * @param dest: output buffer
 * @param dest_size: output size
 * @param hwm: high watermark
 * @return int: size of the result, negative integer on failure
 */
int hwm_to_string(char *dest, size_t dest_size, high_watermark_t const *const hwm);

/**
 * @brief Converts hwm status to string (Enabled/Disabled)
 * @param dest output buffer
 * @param dest_size output size >= 9u
 * @param hwm_disabled High watermark status from NVRAM
 * @return size of the result, negative integer on failure.
 */
int hwm_status_to_string(char *dest, size_t dest_size, volatile bool const *hwm_disabled);

/**
 * @brief Copies a string in a buffer.
 *
 *        Returns -1 if the string is too large to fit in the buffer
 *
 * @param dest: output buffer
 * @param dest_size: size of the output buffer
 * @param src: string to copy
 * @return int: size of the result, negative integer on failure
 */
int copy_string(char *const dest, size_t const dest_size, char const *const src);
