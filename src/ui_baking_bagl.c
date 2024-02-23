#ifdef HAVE_BAGL

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

void ui_baking_reset(__attribute__((unused)) volatile uint32_t* flags) {
    init_screen_stack();
    push_ui_callback("Reset HWM", number_to_string_indirect32, &G.reset_level);

    ux_confirm_screen(reset_ok, delay_reject);
}

#endif  // HAVE_BAGL
