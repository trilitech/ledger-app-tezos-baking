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

#include "apdu.h"
#include "globals.h"
#include "memory.h"
#include "ui.h"

void app_main(void);

void app_main(void) {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;

    init_globals();
    ui_initial_screen();

    for (;;) {
        BEGIN_TRY {
            TRY {
                // Receive command bytes in G_io_apdu_buffer
                input_len = io_recv_command();
                if (input_len < 0) {
                    PRINTF("=> io_recv_command failure\n");
                    return;
                }

                // Parse APDU command from G_io_apdu_buffer
                if (!apdu_parser(&cmd, G_io_apdu_buffer, input_len)) {
                    PRINTF("=> /!\\ BAD LENGTH: %.*H\n", input_len, G_io_apdu_buffer);
                    THROW(EXC_WRONG_LENGTH);
                }

                PRINTF("=> CLA=%02X | INS=%02X | P1=%02X | P2=%02X | Lc=%02X | CData=%.*H\n",
                       cmd.cla,
                       cmd.ins,
                       cmd.p1,
                       cmd.p2,
                       cmd.lc,
                       cmd.lc,
                       cmd.data);

                // Dispatch structured APDU command to handler
                if (apdu_dispatcher(&cmd) < 0) {
                    PRINTF("=> apdu_dispatcher failure\n");
                    return;
                }
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
                io_send_sw(sw);
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}
