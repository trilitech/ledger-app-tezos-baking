#pragma once

#include <string.h>

#include "exception.h"

#define COMPARE(a, b)                                                         \
    ({                                                                        \
        _Static_assert(sizeof(*a) == sizeof(*b), "Size mismatch in COMPARE"); \
        check_null(a);                                                        \
        check_null(b);                                                        \
        memcmp(a, b, sizeof(*a));                                             \
    })
#define NUM_ELEMENTS(a) (sizeof(a) / sizeof(*a))
