#pragma once

#include "object.h"
#include "parser.h"
#include "env.h"

struct object *eval_program(struct program *prog, struct environment *env);