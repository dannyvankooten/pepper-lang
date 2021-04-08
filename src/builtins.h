#pragma once

#include <stdint.h>

struct object *get_builtin(char *name);
struct object *get_builtin_by_index(uint8_t index);
