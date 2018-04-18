/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "os.h"
#include "cx.h"
#include <stdbool.h>

#include "os_io_seproxyhal.h"
#include "string.h"

const bagl_element_t* io_seproxyhal_touch_exit(const bagl_element_t *e);
const bagl_element_t* io_seproxyhal_touch_sign_ok(const bagl_element_t *e);
const bagl_element_t* io_seproxyhal_touch_sign_cancel(const bagl_element_t *e);
const bagl_element_t* io_seproxyhal_touch_address_ok(const bagl_element_t *e);
const bagl_element_t* io_seproxyhal_touch_address_cancel(const bagl_element_t *e);

#define MAX_BIP32_PATH 10

#define CLA 0x80

#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04

#define P1_FIRST 0x00
#define P1_NEXT 0x01
#define P1_LAST_MARKER 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

#define TEZOS_CURVE CX_CURVE_Ed25519

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

ux_state_t ux;

// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;

#define TEZOS_BUFSIZE 1024

typedef struct operationContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    cx_ecfp_public_key_t publicKey;
    uint8_t depth;
    bool readingElement;
    bool getPublicKey;
    uint8_t lengthBuffer[4];
    uint8_t lengthOffset;
    uint32_t elementLength;

    uint8_t data[TEZOS_BUFSIZE];
    uint32_t datalen;
} operationContext_t;

char keyPath[200];
operationContext_t operationContext;

#include "blue_ui.h"
#include "nanos_ui.h"

void ui_idle(void) {
    if (os_seph_features() &
        SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG) {
        UX_DISPLAY(ui_idle_blue, NULL);
    } else {
        UX_DISPLAY(ui_idle_nanos, NULL);
    }
}

unsigned int ui_approval_ssh_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        switch (element->component.userid) {
        case 1:
            io_seproxyhal_setup_ticker(2000);
            break;
        case 2:
            io_seproxyhal_setup_ticker(3000);
            break;
        }
        return (ux_step == element->component.userid - 1);
    }
    return 1;
}

uint32_t path_item_to_string(char *dest, uint32_t number) {
    uint32_t offset = 0;
    uint32_t startOffset = 0, destOffset = 0;
    uint8_t i;
    uint8_t tmp[11];
    bool hardened = ((number & 0x80000000) != 0);
    number &= 0x7FFFFFFF;
    uint32_t divIndex = 0x3b9aca00;
    while (divIndex != 0) {
        tmp[offset++] = '0' + ((number / divIndex) % 10);
        divIndex /= 10;
    }
    tmp[offset] = '\0';
    while ((tmp[startOffset] == '0') && (startOffset < offset)) {
        startOffset++;
    }
    if (startOffset == offset) {
        dest[destOffset++] = '0';
    } else {
        for (i = startOffset; i < offset; i++) {
            dest[destOffset++] = tmp[i];
        }
    }
    if (hardened) {
        dest[destOffset++] = '\'';
    }
    dest[destOffset++] = '\0';
    return destOffset;
}

int path_to_string(char *buf, const operationContext_t *ctx) {
    int i;
    int offset = 0;
    for (i = 0; i < ctx->pathLength; i++) {
        offset +=
            path_item_to_string(buf + offset, ctx->bip32Path[i]);
        offset--;
        if (i != ctx->pathLength - 1) {
            buf[offset++] = '/';
        }
    }
    buf[offset++] = '\0';
    return offset;
}

const bagl_element_t* io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return 0; // do not redraw the widget
}

unsigned int ui_idle_nanos_button(unsigned int button_mask,
                                  unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // EXIT
        io_seproxyhal_touch_exit(NULL);
        break;
    }
    return 0;
}

const bagl_element_t* io_seproxyhal_touch_sign_ok(const bagl_element_t *e) {
    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    int tx = 0;

    os_perso_derive_node_bip32(TEZOS_CURVE,
                               operationContext.bip32Path,
                               operationContext.pathLength,
                               privateKeyData, NULL);

    cx_ecfp_init_private_key(TEZOS_CURVE,
                             privateKeyData, 32, &privateKey);

    os_memset(privateKeyData, 0, sizeof(privateKeyData));

    tx = cx_eddsa_sign(&privateKey,
                       0,
                       CX_SHA512,
                       operationContext.data,
                       operationContext.datalen,
                       NULL,
                       0,
                       G_io_apdu_buffer,
                       64,
                       NULL);

    os_memset(&privateKey, 0, sizeof(privateKey));

    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    // Display back the original UX
    ui_idle();

    return 0; // do not redraw the widget
}

const bagl_element_t* io_seproxyhal_touch_sign_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);

    // Display back the original UX
    ui_idle();

    return 0; // do not redraw the widget
}

unsigned int ui_approval_ssh_nanos_button(unsigned int button_mask,
                                          unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_sign_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_sign_ok(NULL);
        break;
    }
    }
    return 0;
}

unsigned int ui_approval_pgp_nanos_button(unsigned int button_mask,
                                          unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_sign_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_sign_ok(NULL);
        break;
    }
    }
    return 0;
}

const bagl_element_t* io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    int tx = 0;
    cx_edward_compress_point(TEZOS_CURVE,
                             operationContext.publicKey.W,
                             operationContext.publicKey.W_len);
    G_io_apdu_buffer[tx++] = 33;
    os_memmove(G_io_apdu_buffer + tx, operationContext.publicKey.W, 33);
    tx += 33;
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

const bagl_element_t* io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int ui_address_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_address_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_address_ok(NULL);
        break;
    }
    }
    return 0;
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

unsigned int read_bip32_path(operationContext_t *ctx, const uint8_t *buf) {
    int i, nbWritten = 0;
    ctx->pathLength = buf[0];
    buf++;
    nbWritten++;
    if ((operationContext.pathLength < 0x01) ||
        (operationContext.pathLength > MAX_BIP32_PATH)) {
        screen_printf("Invalid path\n");
        THROW(0x6a80);
    }
    for (i = 0; i < ctx->pathLength; i++) {
        ctx->bip32Path[i] =
            (buf[0] << 24) | (buf[1] << 16) |
            (buf[2] << 8) | (buf[3]);
        buf += 4;
        nbWritten++;
    }

    return nbWritten;
}

void sample_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                if (G_io_apdu_buffer[0] != CLA) {
                    THROW(0x6E00);
                }

                switch (G_io_apdu_buffer[1]) {

                case INS_GET_PUBLIC_KEY: {
                    uint8_t privateKeyData[32];
                    uint8_t *dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;
                    cx_ecfp_private_key_t privateKey;

                    if (G_io_apdu_buffer[OFFSET_P1] != 0)
                        THROW(0x6B00);

                    read_bip32_path(&operationContext, dataBuffer);
                    os_perso_derive_node_bip32(TEZOS_CURVE,
                                               operationContext.bip32Path,
                                               operationContext.pathLength,
                                               privateKeyData, NULL);

                    cx_ecfp_init_private_key(TEZOS_CURVE, privateKeyData, 32,
                                             &privateKey);
                    cx_ecfp_generate_pair(TEZOS_CURVE,
                                          &operationContext.publicKey,
                                          &privateKey, 1);

                    os_memset(&privateKey, 0, sizeof(privateKey));
                    os_memset(privateKeyData, 0, sizeof(privateKeyData));

                    path_to_string(keyPath, &operationContext);

                    if (os_seph_features() &
                        SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG) {
                        UX_DISPLAY(ui_address_blue, NULL);
                    } else {
                        UX_DISPLAY(ui_address_nanos, NULL);
                    }

                    flags |= IO_ASYNCH_REPLY;
                }

                break;

                case INS_SIGN: {
                    uint8_t p1 = G_io_apdu_buffer[OFFSET_P1];
                    /* uint8_t p2 = G_io_apdu_buffer[OFFSET_P2]; */
                    uint8_t *dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;
                    uint32_t dataLength = G_io_apdu_buffer[OFFSET_LC];
                    bool last = ((p1 & P1_LAST_MARKER) != 0);
                    p1 &= ~P1_LAST_MARKER;

                    if (p1 == P1_FIRST) {
                        os_memset(operationContext.data, 0, TEZOS_BUFSIZE);
                        operationContext.datalen = 0;
                        dataLength -= read_bip32_path(&operationContext, dataBuffer);
                        THROW(0x9000);
                    }

                    else if (p1 != P1_NEXT)
                        THROW(0x6B00);

                    if (operationContext.datalen + dataLength > TEZOS_BUFSIZE)
                        /* TODO: find a good error code */
                        THROW(0x6C00);

                    os_memmove(operationContext.data+operationContext.datalen,
                               dataBuffer,
                               dataLength);
                    operationContext.datalen += dataLength;

                    if (!last) {
                        THROW(0x9000);
                    }

                    path_to_string(keyPath, &operationContext);

                    if (os_seph_features() &
                        SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG) {
                        UX_DISPLAY(ui_approval_pgp_blue, NULL);
                    } else {
                        UX_DISPLAY(ui_approval_pgp_nanos, NULL);
                    }
                    flags |= IO_ASYNCH_REPLY;
                }

                break;

                case 0xFF: // return to dashboard
                    os_sched_exit(0);

                default:
                    THROW(0x6D00);
                    break;
                }
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                case 0x9000:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}

void io_seproxyhal_display(const bagl_element_t *element) {
    return io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

#ifdef HAVE_BLE
    // Make automatically discoverable again when disconnected

    case SEPROXYHAL_TAG_BLE_CONNECTION_EVENT:
        if (G_io_seproxyhal_spi_buffer[3] == 0) {
            // TODO : cleaner reset sequence
            // first disable BLE before turning it off
            G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_BLE_RADIO_POWER;
            G_io_seproxyhal_spi_buffer[1] = 0;
            G_io_seproxyhal_spi_buffer[2] = 1;
            G_io_seproxyhal_spi_buffer[3] = 0;
            io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 4);
            // send BLE power on (default parameters)
            G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_BLE_RADIO_POWER;
            G_io_seproxyhal_spi_buffer[1] = 0;
            G_io_seproxyhal_spi_buffer[2] = 1;
            G_io_seproxyhal_spi_buffer[3] = 3; // ble on & advertise
            io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 5);
        }
        break;
#endif

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        // prepare next screen
        ux_step = (ux_step + 1) % ux_step_count;
        // redisplay screen
        UX_REDISPLAY();
        break;

    // unknown events are acknowledged
    default:
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    UX_INIT();

    // ensure exception will work as planned
    os_boot();

    BEGIN_TRY {
        TRY {
            io_seproxyhal_init();

            USB_power(1);

            ui_idle();

            sample_main();
        }
        CATCH_OTHER(e) {
        }
        FINALLY {
        }
    }
    END_TRY;

    app_exit();
}
