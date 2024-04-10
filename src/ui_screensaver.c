/* Tezos Ledger application - Screen-saver UI functions

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2023 Ledger

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
#include "ui_screensaver.h"

#include "bolos_target.h"
#include "globals.h"

#define G_screensaver_state global.dynamic_display.screensaver_state

/**
 * @brief Blank screen element
 *
 */
static const bagl_element_t blank_screen_elements[] = {{{BAGL_RECTANGLE,
                                                         BAGL_NONE,
                                                         0,
                                                         0,
                                                         BAGL_WIDTH,
                                                         BAGL_HEIGHT,
                                                         0,
                                                         0,
                                                         BAGL_FILL,
                                                         0x000000,
                                                         0xFFFFFF,
                                                         0,
                                                         0},
                                                        .text = NULL},
                                                       {}};

/**
 * @brief This structure represents the parameters needed for the blank layout
 *
 *        No parameters required
 *
 */
typedef struct ux_layout_blank_params_s {
} ux_layout_blank_params_t;

/**
 * @brief Initializes the blank layout
 *
 * @param stack_slot: index stack_slot
 */
static void ux_layout_blank_init(unsigned int stack_slot) {
    ux_stack_init(stack_slot);
    G_ux.stack[stack_slot].element_arrays[0].element_array = blank_screen_elements;
    G_ux.stack[stack_slot].element_arrays[0].element_array_count = ARRAYLEN(blank_screen_elements);
    G_ux.stack[stack_slot].element_arrays_count = 1;
    G_ux.stack[stack_slot].button_push_callback = ux_flow_button_callback;
    ux_stack_display(stack_slot);
}

/**
 * @brief Exits the blank screen to home screen
 *
 */
static void return_to_idle(void) {
    G_screensaver_state.on = false;
    ui_initial_screen();
}

/**
 * @brief Blank screen flow
 *
 *        On any click: return_to_idle
 *
 */
UX_STEP_CB(blank_screen_step, blank, return_to_idle(), {});
UX_STEP_INIT(blank_screen_border, NULL, NULL, { return_to_idle(); });
UX_FLOW(ux_blank_flow, &blank_screen_step, &blank_screen_border, FLOW_LOOP);

void ui_start_screensaver(void) {
    if (!G_screensaver_state.on) {
        G_screensaver_state.on = true;
        ux_flow_init(0, ux_blank_flow, NULL);
    }
}

#define MS                  100u
#define SCREENSAVER_TIMEOUT 20000u  // 20u * 10u * MS

void ux_screensaver_start_clock(void) {
    G_screensaver_state.clock.on = true;
    G_screensaver_state.clock.timeout = SCREENSAVER_TIMEOUT;
}

void ux_screensaver_stop_clock(void) {
    G_screensaver_state.clock.on = false;
}

void ux_screensaver_apply_tick(void) {
    if (G_screensaver_state.clock.on) {
        if (G_screensaver_state.clock.timeout < MS) {
            ui_start_screensaver();
            ux_screensaver_stop_clock();
        } else {
            G_screensaver_state.clock.timeout -= MS;
        }
    }
}

#endif
