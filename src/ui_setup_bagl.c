#ifdef HAVE_BAGL
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

__attribute__((noreturn)) void prompt_setup(ui_callback_t const ok_cb,
                                            ui_callback_t const cxl_cb) {
    init_screen_stack();
    push_ui_callback("Setup", copy_string, "Baking?");
    push_ui_callback("Address", bip32_path_with_curve_to_pkh_string, &global.path_with_curve);
    push_ui_callback("Chain", chain_id_to_string_with_aliases, &G.main_chain_id);
    push_ui_callback("Main Chain HWM", number_to_string_indirect32, &G.hwm.main);
    push_ui_callback("Test Chain HWM", number_to_string_indirect32, &G.hwm.test);

    ux_confirm_screen(ok_cb, cxl_cb);
}

#endif  // #ifdef BAKING_APP
#endif // HAVE_BAGL
