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

static void confirmation_callback(bool confirm) {
  if (confirm) {
    reset_ok();
    nbgl_useCaseStatus("RESET\nCONFIRMED", true, ui_initial_screen);
  } else {
    delay_reject();
    nbgl_useCaseStatus("Reset cancelled", false, ui_initial_screen);
  }
}

static void cancel_callback(void) { confirmation_callback(false); }

static void continue_light_callback(void) {
  transactionContext.tagValueList.pairs = transactionContext.tagValuePair;

  transactionContext.infoLongPress.icon = &C_tezos;
  transactionContext.infoLongPress.longPressText = "Approve";
  transactionContext.infoLongPress.tuneId = TUNE_TAP_CASUAL;
  transactionContext.infoLongPress.text = "Confirm HWM reset";

  nbgl_useCaseStaticReviewLight(&transactionContext.tagValueList,
                                &transactionContext.infoLongPress, "Cancel",
                                confirmation_callback);
}

void ui_baking_reset(volatile uint32_t *flags) {
  number_to_string_indirect32(transactionContext.buffer,
                              sizeof(transactionContext.buffer),
                              &G.reset_level);

  transactionContext.tagValuePair[0].item = "Reset level";
  transactionContext.tagValuePair[0].value = transactionContext.buffer;

  transactionContext.tagValueList.nbPairs = 1;

  nbgl_useCaseReviewStart(&C_tezos, "Reset HWM", NULL, "Cancel",
                          continue_light_callback, cancel_callback);
  *flags = IO_ASYNCH_REPLY;
}

#endif // #ifdef BAKING_APP
#endif // HAVE_NBGL
