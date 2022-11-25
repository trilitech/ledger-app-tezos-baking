#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
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

#define G global.apdu.u.baking

typedef struct {
    char buffer[sizeof(cx_ecfp_public_key_t)];
    ui_callback_t ok_cb;
    ui_callback_t cxl_cb;
    nbgl_pageInfoLongPress_t infoLongPress;
} TransactionContext_t;

static TransactionContext_t transactionContext;

static void cancel_callback(void) {
    transactionContext.cxl_cb();
    nbgl_useCaseStatus("Address rejected", false, ui_initial_screen);
}

static void approve_callback(void) {
    transactionContext.ok_cb();
    nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, ui_initial_screen);
}

static void confirmation_callback(bool confirm) {
    if (confirm) {
        approve_callback();
    }
    else {
        cancel_callback();
    }
}

static void verify_address(void) {
    nbgl_useCaseAddressConfirmation(transactionContext.buffer, confirmation_callback);
}

void prompt_address(
#ifndef BAKING_APP
    __attribute__((unused))
#endif
    bool baking,
    ui_callback_t ok_cb,
    ui_callback_t cxl_cb) {

    transactionContext.ok_cb = ok_cb;
    transactionContext.cxl_cb = cxl_cb;

    transactionContext.infoLongPress.icon = &C_tezos;
    transactionContext.infoLongPress.longPressText = "Approve";
    transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;

    bip32_path_with_curve_to_pkh_string(transactionContext.buffer, sizeof(transactionContext.buffer), &global.path_with_curve);
    
    char* text;
#ifdef BAKING_APP
    if (baking) {
        text = "Authorize Tezos\nBaking address";
    } else {
#endif
        text = "Verify Tezos\naddress";
#ifdef BAKING_APP
    }
#endif
    nbgl_useCaseReviewStart(&C_tezos, text, "", "Cancel", verify_address, cancel_callback);
}
#endif // HAVE_NBGL
