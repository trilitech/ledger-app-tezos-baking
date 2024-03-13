/* Tezos Ledger application - Common UI functions

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger
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

#include "ui.h"
#include "ux.h"

#include "globals.h"
#include "os.h"

/**
 * @brief Redefinition of lib_standard_app/io.io_event
 *
 *        In order to disable ticker event handling to prevent
 *        screen-saver from starting
 *
 * @param channel: requested channel
 * @return 1
 */
uint8_t io_event(__attribute__((unused)) unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
#ifdef HAVE_NBGL
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#endif  // HAVE_NBGL

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
#ifdef HAVE_BAGL
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
#endif  // HAVE_BAGL
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
                !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
                  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            __attribute__((fallthrough));
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#ifdef HAVE_BAGL
            UX_DISPLAYED_EVENT({});
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
            UX_DEFAULT_EVENT();
#endif  // HAVE_NBGL
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
#if defined(HAVE_BAGL)
            // Disable ticker event handling to prevent screen saver from starting.
#else
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
#endif  // HAVE_BAGL
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

/**
 * @brief Invalidates the pin to enforce its requirement.
 *
 */
void require_pin(void) {
    os_global_pin_invalidate();
}

void exit_app(void) {
    require_pin();
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);

    THROW(0);  // Suppress warning
}
