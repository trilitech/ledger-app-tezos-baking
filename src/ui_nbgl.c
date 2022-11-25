#ifdef HAVE_NBGL
#include "bolos_target.h"

#include "ui.h"

#include "baking_auth.h"
#include "exception.h"
#include "globals.h"
#include "glyphs.h"  // ui-menu
#include "keys.h"
#include "memory.h"
#include "os_cx.h"  // ui-menu
#include "to_string.h"

#include <stdbool.h>
#include <string.h>

#include "nbgl_use_case.h"

#define MAX_LENGTH 200

static const char* const infoTypes[] = {
    "Version", 
    "Chain", 
    "Public Key Hash", 
    "High Watermark", 
};

static char* infoContents[4];
static char buffer[4][MAX_LENGTH];

void ui_initial_screen(void) {
    ux_idle_screen(NULL, NULL);
}

#ifdef BAKING_APP
static bool navigation_cb_baking(uint8_t page, nbgl_pageContent_t* content) {
    UNUSED(page);

    infoContents[0] = buffer[0];
    infoContents[1] = buffer[1];
    infoContents[2] = buffer[2];
    infoContents[3] = buffer[3];

    copy_string(buffer[0], sizeof(buffer[0]), VERSION);
    copy_chain(buffer[1], sizeof(buffer[1]), &N_data.main_chain_id);
    copy_key(buffer[2], sizeof(buffer[2]), &N_data.baking_key);
    copy_hwm(buffer[3], sizeof(buffer[3]), &N_data.hwm.main);
        
    if (page == 0) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 3;
        content->infosList.infoTypes = &infoTypes[0];
        content->infosList.infoContents = &infoContents[0];
    }
    else {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 1;
        content->infosList.infoTypes = &infoTypes[3];
        content->infosList.infoContents = &infoContents[3];
    }

    return true;
}

void ui_menu_about_baking(void) {
    nbgl_useCaseSettings("Tezos baking", 0, 2, false, ui_initial_screen, navigation_cb_baking, NULL);
}

#else
static bool navigation_cb_wallet(uint8_t page, nbgl_pageContent_t* content) {
    UNUSED(page);

    infoContents[0] = buffer[0];

    copy_string(buffer[0], sizeof(buffer[0]), VERSION);
        
    if (page == 0) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 1;
        content->infosList.infoTypes = &infoTypes[0];
        content->infosList.infoContents = &infoContents[0];
    }

    return true;
}

void ui_menu_about_wallet(void) {
    nbgl_useCaseSettings("Tezos wallet", 0, 1, false, ui_initial_screen, navigation_cb_wallet, NULL);
}

#endif

void ux_idle_screen(ui_callback_t ok_c, ui_callback_t cxl_c) {
    (void) ok_c;
    (void) cxl_c;

#ifdef BAKING_APP
    nbgl_useCaseHome("Tezos Baking", &C_tezos, NULL, false, ui_menu_about_baking, exit_app);
#else
    nbgl_useCaseHome("Tezos", &C_tezos, NULL, false, ui_menu_about_wallet, exit_app);
#endif
}

#endif // HAVE_NBGL
