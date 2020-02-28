#include "code/code.h"
#include "eval/object.h"

struct compiler {
    struct instruction *instructions;
    struct object_list *constants;
};

struct bytecode {
    struct instruction *instructions;
    struct object_list *constants;
};

struct compiler *make_compiler();
int compile(struct compiler *compiler, struct program *program);
struct bytecode *get_bytecode(struct compiler *c);
void concat_instructions(struct instruction *ins1, struct instruction *ins2);
