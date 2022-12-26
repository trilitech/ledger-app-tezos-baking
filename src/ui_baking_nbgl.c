#ifdef HAVE_NBGL
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

#include "nbgl_use_case.h"
#define G global.apdu.u.baking

typedef struct {
    nbgl_layoutTagValue_t tagValuePair[1];
    nbgl_layoutTagValueList_t tagValueList;
    nbgl_pageInfoLongPress_t infoLongPress;
    char buffer[MAX_INT_DIGITS + 1];
} TransactionContext_t;

static TransactionContext_t transactionContext;

static bool approve_callback(void) {
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

static void confirmation_callback(bool confirm) {
    if (confirm) {
        approve_callback();
        nbgl_useCaseStatus("RESET\nCONFIRMED", true, ui_initial_screen);
    }
    else {
        delay_reject();
        nbgl_useCaseStatus("Reset cancelled", false, ui_initial_screen);
    }
}

static void cancel_callback(void) {
    confirmation_callback(false);
}

static void continue_light_callback(void) {
    transactionContext.tagValueList.pairs = transactionContext.tagValuePair;

    transactionContext.infoLongPress.icon = &C_tezos;
    transactionContext.infoLongPress.longPressText = "Approve";
    transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;
    transactionContext.infoLongPress.text = "Confirm HWM reset";

    nbgl_useCaseStaticReviewLight(&transactionContext.tagValueList, &transactionContext.infoLongPress, "Cancel", confirmation_callback);
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

    number_to_string_indirect32(transactionContext.buffer, sizeof(transactionContext.buffer), &G.reset_level);

    transactionContext.tagValuePair[0].item = "Reset level";
    transactionContext.tagValuePair[0].value = transactionContext.buffer;

    transactionContext.tagValueList.nbPairs = 1;

    nbgl_useCaseReviewStart(&C_tezos, "Reset HWM", NULL, "Cancel", continue_light_callback, cancel_callback);
    *flags = IO_ASYNCH_REPLY;
    return 0;
}

#endif  // #ifdef BAKING_APP
#endif // HAVE_NBGL
