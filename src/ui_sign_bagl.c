#ifdef HAVE_BAGL
#include "apdu_sign.h"

#include "apdu.h"
#include "baking_auth.h"
#include "base58_encoding.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

#define B2B_BLOCKBYTES 128

__attribute__((noreturn)) void prompt_register_delegate(ui_callback_t const ok_cb,
                                                        ui_callback_t const cxl_cb) {
    if (!G.maybe_ops.is_valid) THROW(EXC_MEMORY_ERROR);

    init_screen_stack();
    push_ui_callback("Register", copy_string, "as delegate?");
    push_ui_callback("Address", bip32_path_with_curve_to_pkh_string, &global.path_with_curve);
    push_ui_callback("Fee", microtez_to_string_indirect, &G.maybe_ops.v.total_fee);

    ux_confirm_screen(ok_cb, cxl_cb);
    __builtin_unreachable();
}

#endif  // HAVE_BAGL
