#include <stdlib.h>
#include "mem.h"
#include <err.h>

void malloc_init() {
    for (int i=0; i < 128-1; i++) {
        mem._object[i].next = &mem._object[i+1];
        mem._object_list[i].next = &mem._object_list[i+1];
    }

    mem.object = &mem._object[0];
    mem.object_list = &mem._object_list[0];
};

struct object *malloc_object() {
   struct object *tmp = mem.object;
   mem.object = tmp->next;
   return tmp;
}

void malloc_free_object(struct object *obj) {
    obj->next = mem.object;
    mem.object = obj;
}

struct object_list *malloc_object_list(unsigned int cap) {
   struct object_list *tmp = mem.object_list;
   mem.object_list = tmp->next;

    if (tmp->cap < cap) {
        tmp->values = realloc(tmp->values, sizeof *tmp->values * cap);
        if (!tmp->values) {
            err(EXIT_FAILURE, "out of memory");
        }
        tmp->cap = cap;
    }

   return tmp;
}

void malloc_free_object_list(struct object_list *list) {
    list->next = mem.object_list;
    mem.object_list = list;
}