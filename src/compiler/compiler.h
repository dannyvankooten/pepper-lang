#include "code/code.h"
#include "eval/object.h"

struct compiler {
    struct instruction instructions[64];
    unsigned int instructions_size;
    struct object constants[64];
    unsigned int constants_size;
};

struct bytecode {
    struct instruction instructions[64];
    unsigned int instructions_size;
    struct object constants[64];
    unsigned int constants_size;
};

int
compile(struct compiler *compiler, struct program *program);

struct bytecode *
get_bytecode(struct compiler *c);