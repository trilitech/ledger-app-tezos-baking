#include "ui.h"
#include "ux.h"

#include "globals.h"
#include "os.h"

#ifdef HAVE_BAGL
void io_seproxyhal_display(const bagl_element_t *element);

void io_seproxyhal_display(const bagl_element_t *element) {
    return io_seproxyhal_display_default((bagl_element_t *) element);
}

void ui_refresh(void) {
    ux_stack_display(0);
}
#endif // HAVE_BAGL

// CALLED BY THE SDK
unsigned char io_event(unsigned char channel);

unsigned char io_event(__attribute__((unused)) unsigned char channel) {
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
            // no break is intentional
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#ifdef HAVE_BAGL
            UX_DISPLAYED_EVENT({});
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
            UX_DEFAULT_EVENT();
#endif // HAVE_NBGL
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
#if defined (HAVE_BAGL) && defined (BAKING_APP)  
            // Disable ticker event handling to prevent screen saver from starting.
#else
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
#endif // HAVE_BAGL
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}
void ui_init(void) {
#ifdef HAVE_BAGL
        UX_INIT();
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
        nbgl_objInit();
#endif  // HAVE_NBGL
}

void require_pin(void) {
    os_global_pin_invalidate();
}

bool exit_app(void) {
#ifdef BAKING_APP
    require_pin();
#endif
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
