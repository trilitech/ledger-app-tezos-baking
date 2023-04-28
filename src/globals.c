#include "globals.h"

#include "exception.h"
#include "to_string.h"

#include "ux.h"

#include <string.h>

// WARNING: ***************************************************
// Non-const globals MUST NOT HAVE AN INITIALIZER.
//
// Providing an initializer will cause the application to crash
// if you write to it.
// ************************************************************

globals_t global;

// These are strange variables that the SDK relies on us to define but uses directly itself.
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

void clear_apdu_globals(void) {
    memset(&global.apdu, 0, sizeof(global.apdu));
}

void init_globals(void) {
    memset(&global, 0, sizeof(global));

    memset(&G_ux, 0, sizeof(G_ux));
    memset(&G_ux_params, 0, sizeof(G_ux_params));

    memset(G_io_seproxyhal_spi_buffer, 0, sizeof(G_io_seproxyhal_spi_buffer));
}

#ifdef BAKING_APP

// DO NOT TRY TO INIT THIS. This can only be written via an system call.
// The "N_" is *significant*. It tells the linker to put this in NVRAM.
nvram_data const N_data_real;

high_watermark_t volatile *select_hwm_by_chain(chain_id_t const chain_id,
                                               nvram_data volatile *const ram) {
    check_null(ram);
    return chain_id.v == ram->main_chain_id.v || ram->main_chain_id.v == 0 ? &ram->hwm.main
                                                                           : &ram->hwm.test;
}

void copy_chain(char *out, size_t out_size, void *data) {
    chain_id_t *chain_id = (chain_id_t *) data;

    if (chain_id->v == 0) {
        copy_string(out, out_size, "any");
    } else {
        chain_id_to_string_with_aliases(out, out_size, (chain_id_t const *const) chain_id);
    }
}

void copy_key(char *out, size_t out_size, void *data) {
    bip32_path_with_curve_t *baking_key = (bip32_path_with_curve_t *) data;
    if (baking_key->bip32_path.length == 0) {
        copy_string(out, out_size, "No Key Authorized");
    } else {
        cx_ecfp_public_key_t pubkey = {0};
        generate_public_key(&pubkey,
                            (derivation_type_t const) baking_key->derivation_type,
                            (bip32_path_t const *const) & baking_key->bip32_path);
        pubkey_to_pkh_string(out,
                             out_size,
                             (derivation_type_t const) baking_key->derivation_type,
                             &pubkey);
    }
}

void copy_hwm(char *out, size_t out_size, void *data) {
    high_watermark_t *hwm = (high_watermark_t *) data;
    (void) out_size;

    if (hwm->migrated_to_tenderbake) {
        size_t len1 = number_to_string(out, hwm->highest_level);
        out[len1] = ' ';
        number_to_string(out + len1 + 1, hwm->highest_round);
    } else {
        number_to_string(out, hwm->highest_level);
    }
}

#endif  // #ifdef BAKING_APP
