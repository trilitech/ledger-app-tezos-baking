#ifdef HAVE_BAGL
#include "apdu_pubkey.h"

#include "apdu.h"
#include "cx.h"
#include "globals.h"
#include "keys.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "baking_auth.h"

#include <string.h>

__attribute__((noreturn)) void prompt_address(bool baking,
                                              ui_callback_t ok_cb,
                                              ui_callback_t cxl_cb) {
    init_screen_stack();

    if (baking) {
        push_ui_callback("Authorize Baking", copy_string, "With Public Key?");
        push_ui_callback("Public Key Hash",
                         bip32_path_with_curve_to_pkh_string,
                         &global.path_with_curve);
    } else {
        push_ui_callback("Provide", copy_string, "Public Key");
        push_ui_callback("Public Key Hash",
                         bip32_path_with_curve_to_pkh_string,
                         &global.path_with_curve);
    }

    ux_confirm_screen(ok_cb, cxl_cb);
    __builtin_unreachable();
}
#endif  // HAVE_BAGL
