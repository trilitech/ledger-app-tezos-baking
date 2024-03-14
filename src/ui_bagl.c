/* Tezos Ledger application - Common BAGL UI functions

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

#ifdef HAVE_BAGL
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
#include "ui_empty.h"

#include <stdbool.h>
#include <string.h>

#define G global.ui

#define G_display global.dynamic_display

void init_screen_stack(void) {
    explicit_bzero(&G_display.screen_stack, sizeof(G_display.screen_stack));
    G_display.formatter_index = 0;
    G_display.screen_stack_size = 0;
    G_display.current_state = STATIC_SCREEN;
}

/**
 * @brief Prepare the display
 *
 * @param ok_c: accept callback
 * @param cxl_c: cancel callback
 */
static void ux_prepare_display(ui_callback_t ok_c, ui_callback_t cxl_c) {
    G_display.screen_stack_size = G_display.formatter_index;
    G_display.formatter_index = 0;
    G_display.current_state = STATIC_SCREEN;

    if (ok_c) {
        G_display.ok_callback = ok_c;
    }
    if (cxl_c) {
        G_display.cxl_callback = cxl_c;
    }
}

void push_ui_callback(const char *title, string_generation_callback cb, const void *data) {
    if ((G_display.formatter_index + 1u) >= MAX_SCREEN_STACK_SIZE) {
        THROW(0x6124);
    }
    struct screen_data *fmt = &G_display.screen_stack[G_display.formatter_index];

    fmt->title = title;
    fmt->callback_fn = cb;
    fmt->data = data;
    G_display.formatter_index++;
    G_display.screen_stack_size++;
}

/**
 * @brief Clear screen related values
 *
 */
static void clear_data(void) {
    explicit_bzero(&G_display.screen_title, sizeof(G_display.screen_title));
    explicit_bzero(&G_display.screen_value, sizeof(G_display.screen_value));
}

/**
 * @brief Fills the screen with the data in the `screen_stack` pointed
 *        by the index `G_display.formatter_index`. Fills the
 *        `screen_title` by copying the `.title` field and fills the
 *        `screen_value` by computing `callback_fn` with the `.data`
 *        field as a parameter
 *
 */
static void set_screen_data(void) {
    struct screen_data *fmt = &G_display.screen_stack[G_display.formatter_index];
    if (fmt->title == NULL) {
        // Avoid seg faulting for bad reasons...
        G_display.formatter_index = 0;
        fmt = &G_display.screen_stack[0];
    }
    clear_data();
    copy_string((char *) G_display.screen_title, sizeof(G_display.screen_title), fmt->title);
    fmt->callback_fn(G_display.screen_value, sizeof(G_display.screen_value), fmt->data);
}

/**
 * @brief Enables coherent behavior on bnnn_paging when there are
 *        multiple screens.
 *
 */
static void update_layout(void) {
    G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
        G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
    G_ux.flow_stack[G_ux.stack_count - 1].index--;
    ux_flow_relayout();
}

/**
 * @brief Selects the next screen to display
 *
 *        It allows to navigate through the screens in the order in
 *        which they were pushed.
 *
 *        The left-hand side shows the first screens pushed, the
 *        right-hand side the last.
 *
 *        Goes back to standard navigation by leaving the boundaries.
 *
 * @param is_left_ux_step: if come from the left screen
 */
static void display_next_state(bool is_left_ux_step) {
    if (is_left_ux_step) {  // We're called from the LEFT ux step
        if (G_display.current_state == STATIC_SCREEN) {
            // The previous screen was a static screen, so we're now entering the stack (from the
            // start of the stack).

            // Since we're in the stack, set the state to DYNAMIC_SCREEN.
            G_display.current_state = DYNAMIC_SCREEN;
            // We're just entering the stack so the `formatter_index` is set to 0.
            G_display.formatter_index = 0;
            // Update the screen data.
            set_screen_data();
            // Move to the next step, which will display the screen.
            ux_flow_next();
        } else {
            // The previous screen was NOT a static screen, so we were already in the stack.
            if (G_display.formatter_index == 0u) {
                // If `formatter_index` is 0 then we need to exit the dynamic display.

                // Update the current_state accordingly.
                G_display.current_state = STATIC_SCREEN;
                // Don't need to update the screen data, simply display the ux_flow_prev() which
                // will be a static screen.
                ux_flow_prev();
            } else {
                // The user is still browsing the stack: simply decrement `formatter_index` and
                // update `screen_data`.
                G_display.formatter_index--;
                set_screen_data();
                ux_flow_next();
            }
        }
    } else {
        // We're called from the RIGHT ux step.
        if (G_display.current_state == STATIC_SCREEN) {
            // We're called from a static screen, so we're now entering the stack (from the end of
            // the stack).

            // Update the current_state.
            G_display.current_state = DYNAMIC_SCREEN;
            // We're starting the stack from the end, so the index is the size - 1.
            G_display.formatter_index = G_display.screen_stack_size - 1u;
            // Update the screen data.
            set_screen_data();
            // Since we're called from the RIGHT ux step, if we wish to display
            // the data we need to call ux_flow_PREV and not `ux_flow_NEXT`.
            ux_flow_prev();
        } else {
            // We're being called from a dynamic screen, so the user was already browsing the stack.
            // The user wants to go to the next screen so increment the index.
            G_display.formatter_index++;
            if (G_display.formatter_index == G_display.screen_stack_size) {
                // Index is equal to stack size, so time to exit the stack and display the static
                // screens.

                // Make sure we update the state accordingly.
                G_display.current_state = STATIC_SCREEN;
                ux_flow_next();
            } else {
                // We're just browsing the stack so update the screen and display it.
                set_screen_data();
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                update_layout();
            }
        }
    }
}

/**
 * @brief Generic way to display screens
 *
 *        The border helps to stay in the variable_display page
 *        See `display_next_state(is_left_ux_step)`
 *
 */
UX_STEP_INIT(ux_init_upper_border, NULL, NULL, { display_next_state(true); });
UX_STEP_NOCB(ux_variable_display,
             bnnn_paging,
             (const ux_layout_bnnn_paging_params_t){
                 .title = G_display.screen_title,
                 .text = G_display.screen_value,
             });
UX_STEP_INIT(ux_init_lower_border, NULL, NULL, { display_next_state(false); });

/**
 * @brief Idle flow
 *
 *        - Home screen
 *        - Version screen
 *        - Chain-id screen
 *        - Public key hash screen
 *        - High Watermark screen
 *        - Exit screen
 *
 */
UX_STEP_CB(ux_app_is_ready_step,
           nn,
           ux_empty_screen(),
           {
               "Application",
               "is ready",
           });
UX_STEP_CB(ux_idle_quit_step,
           pb,
           exit_app(),
           {
               &C_icon_dashboard_x,
               "Quit",
           });
UX_FLOW(ux_idle_flow,
        &ux_app_is_ready_step,

        &ux_init_upper_border,
        &ux_variable_display,
        &ux_init_lower_border,

        &ux_idle_quit_step,
        FLOW_LOOP);

/**
 * @brief Pushes the baking screens
 *
 */
static void calculate_baking_idle_screens_data(void) {
    push_ui_callback("Tezos Baking", copy_string, VERSION);
    push_ui_callback("Chain", copy_chain, &N_data.main_chain_id);
    push_ui_callback("Public Key Hash", copy_key, &N_data.baking_key);
    push_ui_callback("High Watermark", copy_hwm, &N_data.hwm.main);
}

void ui_initial_screen(void) {
    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    init_screen_stack();
    calculate_baking_idle_screens_data();

    ux_prepare_display(NULL, NULL);
    ux_flow_init(0, ux_idle_flow, NULL);
}

void update_baking_idle_screens(void) {
    init_screen_stack();
    calculate_baking_idle_screens_data();
    /// refresh
    ux_stack_display(0);
}

/**
 * @brief Callback called on accept or cancel
 *
 * @param accepted: true if accepted, false if cancelled
 */
static void prompt_response(bool const accepted) {
    ui_initial_screen();
    if (accepted) {
        G_display.ok_callback();
    } else {
        G_display.cxl_callback();
    }
}

/**
 * @brief Confirmation flow
 *
 *        - Initial screen
 *        - Values
 *        - Reject screen
 *        - Accept screen
 *
 */
UX_STEP_CB(ux_prompt_flow_reject_step, pb, prompt_response(false), {&C_icon_crossmark, "Reject"});
UX_STEP_CB(ux_prompt_flow_accept_step, pb, prompt_response(true), {&C_icon_validate_14, "Accept"});
UX_STEP_NOCB(ux_eye_step,
             nn,
             {
                 "Review",
                 "Request",
             });
UX_FLOW(ux_confirm_flow,
        &ux_eye_step,

        &ux_init_upper_border,
        &ux_variable_display,
        &ux_init_lower_border,

        &ux_prompt_flow_reject_step,
        &ux_prompt_flow_accept_step);

void ux_confirm_screen(ui_callback_t ok_c, ui_callback_t cxl_c) {
    ux_prepare_display(ok_c, cxl_c);
    ux_flow_init(0, ux_confirm_flow, NULL);
    THROW(ASYNC_EXCEPTION);
}

#endif  // HAVE_BAGL
