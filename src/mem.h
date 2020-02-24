#ifndef MEM_H
#define MEM_H

#include "object.h"

struct mem {
    struct object _object[128];
    struct object *object;
    struct object_list _object_list[128];
    struct object_list *object_list;
} mem;

void malloc_init();
struct object *malloc_object();
void malloc_free_object(struct object *obj);
struct object_list *malloc_object_list();
void malloc_free_object_list(struct object_list *obj);

#endif 