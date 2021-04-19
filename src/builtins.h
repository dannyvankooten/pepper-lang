#pragma once

#include <stdint.h>
#include "object.h"
#include "symbol_table.h"

struct object get_builtin(const char *name);
struct object get_builtin_by_index(const uint8_t index);

void define_builtins(struct symbol_table *t);