#ifndef VISTACK_H
#define VISTACK_H

#include <ctype.h>
#include <stdlib.h>

struct vistack {
    int *vals;
    size_t len;
    size_t max_len;
};

struct vistack *vistack_create();

void vistack_push(struct vistack *, int);

int vistack_pop(struct vistack *);

size_t vistack_get_size(struct vistack *);

int vistack_is_empty(struct vistack *);

void vistack_destroy(struct vistack *);

#endif
