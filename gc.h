#include <stdlib.h>
#include "object.h"
#include "env.h"

struct gc {
    struct object *objects[200];
    unsigned int size;
};

void gc_add(struct gc *gc, struct object *obj) {
    if (!obj || obj->type == OBJ_BOOL || obj->type == OBJ_NULL) {
        return;
    }

    gc->objects[gc->size++] = obj;
};

void gc_mark_env(struct gc *gc, struct environment *env) {

    struct node *node;
    for (int i=0; i < env->cap; i++) {
        node = env->table[i];

        while (node) {
            node->value->gc_mark = 1;
            node = node->next;
        }
    }

    if (env->outer) {
        gc_mark_env(gc, env->outer);
    }
};

void gc_mark_obj(struct gc *gc, struct object *obj) {
    obj->gc_mark = 1;
}

void gc_sweep(struct gc *gc) {
    unsigned int marked[100];

    int j = 0;
    for (int i=0; i < 100; i++) {
        if (gc->objects[i] == NULL) {
            continue;
        }

        // don't free marked objects
        if (gc->objects[i]->gc_mark) {
            gc->objects[i]->gc_mark = 0;
            marked[j++] = i;
            continue;
        } else {
            //free_object(gc->objects[i]);
        }
    }

    gc->size = j;
    for (int i=0; i < 100; i++) {
        if (i < gc->size) {
            gc->objects[i] = gc->objects[marked[i]];  
        } else {
            gc->objects[i] = NULL;
        }
    }

   
}
