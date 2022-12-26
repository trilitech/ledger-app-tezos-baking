#ifdef HAVE_BAGL
#ifdef BAKING_APP

#include "apdu_baking.h"

#include "apdu.h"
#include "baking_auth.h"
#include "globals.h"
#include "os_cx.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"

#include <string.h>

#define G global.apdu.u.baking

static bool reset_ok(void);

bool reset_ok(void) {
    UPDATE_NVRAM(ram, {
        ram->hwm.main.highest_level = G.reset_level;
        ram->hwm.main.highest_round = 0;
        ram->hwm.main.had_endorsement = false;
        ram->hwm.test.highest_level = G.reset_level;
        ram->hwm.test.highest_round = 0;
        ram->hwm.test.had_endorsement = false;
    });

    // Send back the response, do not restart the event loop
    delayed_send(finalize_successful_send(0));
    return true;
}

size_t handle_apdu_reset(__attribute__((unused)) uint8_t instruction, volatile uint32_t* flags) {
    uint8_t *dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;
    uint32_t dataLength = G_io_apdu_buffer[OFFSET_LC];
    if (dataLength != sizeof(level_t)) {
        THROW(EXC_WRONG_LENGTH_FOR_INS);
    }
    level_t const lvl = READ_UNALIGNED_BIG_ENDIAN(level_t, dataBuffer);
    if (!is_valid_level(lvl)) THROW(EXC_PARSE_ERROR);

    G.reset_level = lvl;

    init_screen_stack();
    push_ui_callback("Reset HWM", number_to_string_indirect32, &G.reset_level);

    ux_confirm_screen(reset_ok, delay_reject);
}

#endif  // #ifdef BAKING_APP
#endif //HAVE_BAGL
