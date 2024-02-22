#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#include "apdu_sign.h"

#include "apdu.h"
#include "baking_auth.h"
#include "base58.h"
#include "globals.h"
#include "keys.h"
#include "memory.h"
#include "protocol.h"
#include "to_string.h"
#include "ui.h"
#include "swap/swap_lib_calls.h"
#include "swap/handle_swap_commands.h"

#include "cx.h"

#include <string.h>

#define G global.apdu.u.sign

#define MAX_LENGTH 100

#define PARSE_ERROR() THROW(EXC_PARSE_ERROR)

typedef struct {
    ui_callback_t ok_cb;
    ui_callback_t cxl_cb;
    nbgl_layoutTagValue_t tagValuePair[6];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    const char* confirmed_status;
    const char* cancelled_status;
    char buffer[6][MAX_LENGTH];
} TransactionContext_t;

static TransactionContext_t transactionContext;

static void cancel_callback(void) {
    nbgl_useCaseStatus(transactionContext.cancelled_status, false, ui_initial_screen);
    transactionContext.cxl_cb();
}

static void approve_callback(void) {
    nbgl_useCaseStatus(transactionContext.confirmed_status, true, ui_initial_screen);
    transactionContext.ok_cb();
}

static void confirmation_callback(bool confirm) {
    if (confirm) {
        approve_callback();
    } else {
        cancel_callback();
    }
}

static void continue_light_callback(void) {
    transactionContext.infoLongPress.icon = &C_tezos;
    transactionContext.infoLongPress.longPressText = "Approve";
    transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReviewLight(&transactionContext.tagValueList,
                                  &transactionContext.infoLongPress,
                                  "Cancel",
                                  confirmation_callback);
}

void prompt_register_delegate(ui_callback_t const ok_cb, ui_callback_t const cxl_cb) {
    if (!G.maybe_ops.is_valid) THROW(EXC_MEMORY_ERROR);

    transactionContext.ok_cb = ok_cb;
    transactionContext.cxl_cb = cxl_cb;

    bip32_path_with_curve_to_pkh_string(transactionContext.buffer[0],
                                        sizeof(transactionContext.buffer[0]),
                                        &global.path_with_curve);
    microtez_to_string_indirect(transactionContext.buffer[1],
                                sizeof(transactionContext.buffer[1]),
                                &G.maybe_ops.v.total_fee);

    transactionContext.tagValuePair[0].item = "Address";
    transactionContext.tagValuePair[0].value = transactionContext.buffer[0];

    transactionContext.tagValuePair[1].item = "Fee";
    transactionContext.tagValuePair[1].value = transactionContext.buffer[1];

    transactionContext.tagValueList.nbPairs = 2;
    transactionContext.tagValueList.pairs = transactionContext.tagValuePair;

    transactionContext.infoLongPress.text = "Confirm delegate\nregistration";

    transactionContext.confirmed_status = "DELEGATE\nCONFIRMED";
    transactionContext.cancelled_status = "Delegate registration\ncancelled";

    nbgl_useCaseReviewStart(&C_tezos,
                            "Register delegate",
                            NULL,
                            "Cancel",
                            continue_light_callback,
                            cancel_callback);
}

#endif  // HAVE_NBGL
