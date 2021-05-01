#pragma once

void gc(struct vm* vm);
void gc_add(struct vm* vm, struct object obj);
void gc_mark(struct object* obj, bool marked);
void gc_sweep(struct object_list* heap);
