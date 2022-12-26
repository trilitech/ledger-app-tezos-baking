#ifdef BAKING_APP

#include "apdu_setup.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.setup

static bool ok(void) {
    UPDATE_NVRAM(ram, {
        copy_bip32_path_with_curve(&ram->baking_key, &global.path_with_curve);
        ram->main_chain_id = G.main_chain_id;
        ram->hwm.main.highest_level = G.hwm.main;
        ram->hwm.main.highest_round = 0;
        ram->hwm.main.had_endorsement = false;
        ram->hwm.main.had_preendorsement = false;
        ram->hwm.main.migrated_to_tenderbake = false;
        ram->hwm.test.highest_level = G.hwm.test;
        ram->hwm.test.highest_round = 0;
        ram->hwm.test.had_endorsement = false;
        ram->hwm.test.had_preendorsement = false;
        ram->hwm.test.migrated_to_tenderbake = false;
    });

    cx_ecfp_public_key_t pubkey = {0};
    generate_public_key(&pubkey,
                        global.path_with_curve.derivation_type,
                        &global.path_with_curve.bip32_path);
    delayed_send(provide_pubkey(G_io_apdu_buffer, &pubkey));
    return true;
}

size_t handle_apdu_setup(__attribute__((unused)) uint8_t instruction) {
    if (G_io_apdu_buffer[OFFSET_P1] != 0) THROW(EXC_WRONG_PARAM);

    uint32_t const buff_size = G_io_apdu_buffer[OFFSET_LC];
    if (buff_size < sizeof(struct setup_wire)) THROW(EXC_WRONG_LENGTH_FOR_INS);

    global.path_with_curve.derivation_type = parse_derivation_type(G_io_apdu_buffer[OFFSET_CURVE]);

    {
        struct setup_wire const *const buff_as_setup =
            (struct setup_wire const *) &G_io_apdu_buffer[OFFSET_CDATA];

        size_t consumed = 0;
        G.main_chain_id.v =
            CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                         uint32_t,
                                         (uint8_t const *) &buff_as_setup->main_chain_id);
        G.hwm.main = CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                                  uint32_t,
                                                  (uint8_t const *) &buff_as_setup->hwm.main);
        G.hwm.test = CONSUME_UNALIGNED_BIG_ENDIAN(consumed,
                                                  uint32_t,
                                                  (uint8_t const *) &buff_as_setup->hwm.test);
        consumed += read_bip32_path(&global.path_with_curve.bip32_path,
                                    (uint8_t const *) &buff_as_setup->bip32_path,
                                    buff_size - consumed);

        if (consumed != buff_size) THROW(EXC_WRONG_LENGTH);
    }

    prompt_setup(ok, delay_reject);
}

#endif  // #ifdef BAKING_APP
