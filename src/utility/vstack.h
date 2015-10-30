#ifndef VSTACK_H
#define VSTACK_H

#include <ctype.h>
#include <stdlib.h>

struct vstack {
    void **vals;
    size_t len;
    size_t max_len;
};

struct vstack *vstack_create();

void vstack_push(struct vstack *, void*);

void *vstack_pop(struct vstack *);

size_t vstack_get_size(struct vstack *);

int vstack_is_empty(struct vstack *s);

void vstack_destroy(struct vstack *);

#endif
