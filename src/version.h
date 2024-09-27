/* Tezos Ledger application - Version handling

   Copyright 2024 TriliTech <contact@trili.tech>
   Copyright 2024 Functori <contact@functori.com>
   Copyright 2018 Obsidian Systems

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

#include <stdint.h>

#define CLASS 1

#ifndef MAJOR_VERSION
#define MAJOR_VERSION 2
#endif

#ifndef MINOR_VERSION
#define MINOR_VERSION 5
#endif

#ifndef PATCH_VERSION
#define PATCH_VERSION 1
#endif

/**
 * @brief This structure represents the version
 *
 */
typedef struct version {
    /// 0 for the wallet app, 1 for the baking app
    uint8_t class;
    uint8_t major;  ///< Major version component
    uint8_t minor;  ///< Minor version component
    uint8_t patch;  ///< Patch version component
} version_t;

/**
 * @brief Constant app version
 *
 */
static const version_t version = {CLASS, MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION};
