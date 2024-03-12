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
#include "ui_empty.h"

#include "bolos_target.h"
#include "globals.h"
#include "ui.h"

const bagl_element_t empty_screen_elements[] = {{{BAGL_RECTANGLE,
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

typedef struct ux_layout_empty_params_s {
} ux_layout_empty_params_t;

void ux_layout_empty_init(unsigned int stack_slot) {
    ux_stack_init(stack_slot);
    G_ux.stack[stack_slot].element_arrays[0].element_array = empty_screen_elements;
    G_ux.stack[stack_slot].element_arrays[0].element_array_count = ARRAYLEN(empty_screen_elements);
    G_ux.stack[stack_slot].element_arrays_count = 1;
    G_ux.stack[stack_slot].button_push_callback = ux_flow_button_callback;
    ux_stack_display(stack_slot);
}

void ux_layout_empty_screen_init(__attribute__((unused)) unsigned int x) {
}

void return_to_idle() {
    global.dynamic_display.is_blank_screen = false;
    ux_idle_screen(NULL, NULL);
}

UX_STEP_CB(empty_screen_step, empty, return_to_idle(), {});
UX_STEP_INIT(empty_screen_border, NULL, NULL, { return_to_idle(); });
UX_FLOW(ux_empty_flow, &empty_screen_step, &empty_screen_border, FLOW_LOOP);

void ux_empty_screen() {
    if (global.dynamic_display.is_blank_screen == false) {
        global.dynamic_display.is_blank_screen = true;
        ux_flow_init(0, ux_empty_flow, NULL);
    }
}
#else
void ux_empty_screen() {
}
#endif
