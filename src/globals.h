#pragma once

#include "types.h"

#include "bolos_target.h"

#include "operations.h"

// Zeros out all globals that can keep track of APDU instruction state.
// Notably this does *not* include UI state.
void clear_apdu_globals(void);

void copy_chain(char *out, size_t out_size, void *data);
void copy_key(char *out, size_t out_size, void *data);
void copy_hwm(char *out, size_t out_size, void *data);
// Zeros out all application-specific globals and SDK-specific UI/exchange buffers.
void init_globals(void);

#define MAX_APDU_SIZE 235  // Maximum number of bytes in a single APDU

// Our buffer must accommodate any remainder from hashing and the next message at once.
#define TEZOS_BUFSIZE (BLAKE2B_BLOCKBYTES + MAX_APDU_SIZE)

#define PRIVATE_KEY_DATA_SIZE 64

#define MAX_SIGNATURE_SIZE 100

#ifdef BAKING_APP
typedef struct {
    uint8_t signed_hmac_key[MAX_SIGNATURE_SIZE];
    uint8_t hashed_signed_hmac_key[CX_SHA512_SIZE];
    uint8_t hmac[CX_SHA256_SIZE];
} apdu_hmac_state_t;
#endif

typedef struct {
    cx_blake2b_t state;
    bool initialized;
} blake2b_hash_state_t;

typedef struct {
    uint8_t packet_index;  // 0-index is the initial setup packet, 1 is first packet to hash, etc.

#ifdef BAKING_APP
    parsed_baking_data_t parsed_baking_data;
#endif

    struct {
        bool is_valid;
        struct parsed_operation_group v;
    } maybe_ops;

    uint8_t message_data[TEZOS_BUFSIZE];
    size_t message_data_length;
    buffer_t message_data_as_buffer;

    blake2b_hash_state_t hash_state;
    uint8_t final_hash[SIGN_HASH_SIZE];

    uint8_t magic_byte;
    bool hash_only;
    struct parse_state parse_state;
} apdu_sign_state_t;

// Used to compute what we need to display on the screen.
// Title of the screen will be `title` field, and value of
// the screen will be generated by calling `callback_fn` and providing
// `data` as one of its parameter.
struct screen_data {
    char *title;
    string_generation_callback callback_fn;
    void *data;
};

// State of the dynamic display.
// Used to keep track on whether we are displaying screens inside the stack,
// or outside the stack (for example confirmation screens).
enum e_state {
    STATIC_SCREEN,
    DYNAMIC_SCREEN,
};

typedef struct {
    struct {
        struct screen_data screen_stack[MAX_SCREEN_STACK_SIZE];
        enum e_state current_state;  // State of the dynamic display

        // Size of the screen stack
        uint8_t screen_stack_size;

        // Current index in the screen_stack.
        uint8_t formatter_index;

        // Callback function if user accepted prompt.
        ui_callback_t ok_callback;
        // Callback function if user rejected prompt.
        ui_callback_t cxl_callback;

        // Title to be displayed on the screen.
        char screen_title[PROMPT_WIDTH + 1];
        // Value to be displayed on the screen.
        char screen_value[VALUE_WIDTH + 1];
    } dynamic_display;

    void *stack_root;
    apdu_handler handlers[INS_MAX + 1];
    bip32_path_with_curve_t path_with_curve;

    struct {
        union {
            apdu_sign_state_t sign;

#ifdef BAKING_APP
            struct {
                level_t reset_level;
            } baking;

            struct {
                chain_id_t main_chain_id;
                struct {
                    level_t main;
                    level_t test;
                } hwm;
            } setup;

            apdu_hmac_state_t hmac;
#endif
        } u;

#ifdef BAKING_APP
        struct {
            nvram_data new_data;  // Staging area for setting N_data
        } baking_auth;
#endif
    } apdu;
} globals_t;

extern globals_t global;

extern unsigned int app_stack_canary;  // From SDK

extern unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#ifdef BAKING_APP
extern nvram_data const N_data_real;
#define N_data (*(volatile nvram_data *) PIC(&N_data_real))

void update_baking_idle_screens(void);
high_watermark_t volatile *select_hwm_by_chain(chain_id_t const chain_id,
                                               nvram_data volatile *const ram);

// Properly updates NVRAM data to prevent any clobbering of data.
// 'out_param' defines the name of a pointer to the nvram_data struct
// that 'body' can change to apply updates.
#define UPDATE_NVRAM(out_name, body)                                                    \
    ({                                                                                  \
        nvram_data *const out_name = &global.apdu.baking_auth.new_data;                 \
        memcpy(&global.apdu.baking_auth.new_data,                                       \
               (nvram_data const *const) & N_data,                                      \
               sizeof(global.apdu.baking_auth.new_data));                               \
        body;                                                                           \
        nvm_write((void *) &N_data, &global.apdu.baking_auth.new_data, sizeof(N_data)); \
        update_baking_idle_screens();                                                   \
    })
#endif
