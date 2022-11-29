#ifdef HAVE_BAGL
#include "apdu_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#ifdef BAKING_APP
#include "baking_auth.h"
#endif  // BAKING_APP

#include <string.h>

__attribute__((noreturn)) void prompt_address(
#ifndef BAKING_APP
    __attribute__((unused))
#endif
    bool baking,
    ui_callback_t ok_cb,
    ui_callback_t cxl_cb) {
    init_screen_stack();

#ifdef BAKING_APP
    if (baking) {
        push_ui_callback("Authorize Baking", copy_string, "With Public Key?");
        push_ui_callback("Public Key Hash",
                         bip32_path_with_curve_to_pkh_string,
                         &global.path_with_curve);
    } else {
#endif
        push_ui_callback("Provide", copy_string, "Public Key");
        push_ui_callback("Public Key Hash",
                         bip32_path_with_curve_to_pkh_string,
                         &global.path_with_curve);
#ifdef BAKING_APP
    }
#endif

    ux_confirm_screen(ok_cb, cxl_cb);
}
#endif // HAVE_BAGL
