#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "keys.h"
#include "operations.h"
#include "os_cx.h"
#include "types.h"
#include "ui.h"

void pubkey_to_pkh_string(char *const out,
                          size_t const out_size,
                          derivation_type_t const derivation_type,
                          cx_ecfp_public_key_t const *const public_key);
void bip32_path_with_curve_to_pkh_string(char *const out,
                                         size_t const out_size,
                                         bip32_path_with_curve_t const *const key);
void chain_id_to_string_with_aliases(char *const out,
                                     size_t const out_size,
                                     chain_id_t const *const chain_id);

// dest must be at least MAX_INT_DIGITS
size_t number_to_string(char *const dest, uint64_t number);

// These take their number parameter through a pointer and take a length
void number_to_string_indirect32(char *const dest,
                                 size_t const buff_size,
                                 uint32_t const *const number);
void microtez_to_string_indirect(char *const dest,
                                 size_t const buff_size,
                                 uint64_t const *const number);

// `src` may be unrelocated pointer to rodata.
void copy_string(char *const dest, size_t const buff_size, char const *const src);
