/* Tezos Ledger application - Main

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2021 Ledger
   Copyright 2019 Obsidian Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "apdu_hmac.h"
#include "apdu_pubkey.h"
#include "apdu_query.h"
#include "apdu_reset.h"
#include "apdu_setup.h"
#include "apdu_sign.h"
#include "apdu.h"
#include "globals.h"
#include "memory.h"
#include "ui.h"

__attribute__((noreturn)) void app_main(void);

__attribute__((noreturn)) void app_main(void) {
    init_globals();
    ui_initial_screen();

    // TODO: Consider using static initialization of a const, instead of this
    for (size_t i = 0; i < NUM_ELEMENTS(global.handlers); i++) {
        global.handlers[i] = handle_apdu_error;
    }
    global.handlers[APDU_INS(INS_VERSION)] = handle_apdu_version;
    global.handlers[APDU_INS(INS_GET_PUBLIC_KEY)] = handle_apdu_get_public_key;
    global.handlers[APDU_INS(INS_PROMPT_PUBLIC_KEY)] = handle_apdu_get_public_key;
    global.handlers[APDU_INS(INS_SIGN)] = handle_apdu_sign;
    global.handlers[APDU_INS(INS_GIT)] = handle_apdu_git;
    global.handlers[APDU_INS(INS_SIGN_WITH_HASH)] = handle_apdu_sign_with_hash;
    global.handlers[APDU_INS(INS_AUTHORIZE_BAKING)] = handle_apdu_get_public_key;
    global.handlers[APDU_INS(INS_RESET)] = handle_apdu_reset;
    global.handlers[APDU_INS(INS_QUERY_AUTH_KEY)] = handle_apdu_query_auth_key;
    global.handlers[APDU_INS(INS_QUERY_MAIN_HWM)] = handle_apdu_main_hwm;
    global.handlers[APDU_INS(INS_SETUP)] = handle_apdu_setup;
    global.handlers[APDU_INS(INS_QUERY_ALL_HWM)] = handle_apdu_all_hwm;
    global.handlers[APDU_INS(INS_DEAUTHORIZE)] = handle_apdu_deauthorize;
    global.handlers[APDU_INS(INS_QUERY_AUTH_KEY_WITH_CURVE)] =
        handle_apdu_query_auth_key_with_curve;
    global.handlers[APDU_INS(INS_HMAC)] = handle_apdu_hmac;

    volatile size_t rx = io_exchange(CHANNEL_APDU, 0);
    volatile uint32_t flags = 0;
    while (true) {
        BEGIN_TRY {
            TRY {
                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);
                // Process APDU of size rx

                if (!rx) {
                    // no apdu received, well, reset the session, and reset the
                    // bootloader configuration
                    THROW(EXC_SECURITY);
                }

                // The amount of bytes we get in our APDU must match what the APDU declares
                // its own content length is. All these values are unsigned, so this implies
                // that if rx < OFFSET_CDATA it also throws.
                if (rx != (G_io_apdu_buffer[OFFSET_LC] + OFFSET_CDATA)) {
                    THROW(EXC_WRONG_LENGTH);
                }

                size_t const tx = apdu_dispatcher(&flags);
                rx = io_exchange(CHANNEL_APDU | flags, tx);
                flags = 0;
            }
            CATCH(ASYNC_EXCEPTION) {
                rx = io_exchange(CHANNEL_APDU | IO_ASYNCH_REPLY, 0);
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                clear_apdu_globals();  // IMPORTANT: Application state must not persist through
                                       // errors

                uint16_t sw = e;
                PRINTF("Error caught at top level, number: %x\n", sw);
                switch (sw) {
                    case 0x6000 ... 0x6FFF:
                    case 0x9000 ... 0x9FFF:
                        break;
                    default:
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
                PRINTF("Line number: %d", sw & 0x0FFF);
                size_t tx = 0;
                G_io_apdu_buffer[tx] = sw >> 8;
                tx++;
                G_io_apdu_buffer[tx] = sw;
                tx++;
                rx = io_exchange(CHANNEL_APDU, tx);
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}
