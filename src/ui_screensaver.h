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
#pragma once

#include <stdbool.h>

#ifdef HAVE_BAGL

typedef struct {
    /// Clock has started
    bool on;
    /// Timeout out before saving screen
    unsigned int timeout;
} ux_screensaver_clock_t;

typedef struct {
    /// Screensaver is on/off.
    bool on;
    /// Timeout out before saving screen
    ux_screensaver_clock_t clock;
} ux_screensaver_state_t;

/**
 * @brief Empties the screen
 *
 *        Waits a click to return to home screen
 *
 *        Applies only for Nanos devices
 *
 */
void ui_start_screensaver(void);

/**
 * @brief Start a timeout before saving screen
 *
 */
void ux_screensaver_start_clock(void);

/**
 * @brief Stop the clock
 *
 */
void ux_screensaver_stop_clock(void);

/**
 * @brief Apply one tick to the clock
 *
 *        The tick is assumed to be 100 ms
 *
 */
void ux_screensaver_apply_tick(void);

#endif
