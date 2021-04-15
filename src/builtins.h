#pragma once

#include <stdint.h>

struct object get_builtin(const char *name);
struct object get_builtin_by_index(const uint8_t index);
