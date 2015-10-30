#include "vstack.h"
#include <stdlib.h>

struct vstack *vstack_create()
{
    struct vstack *s = malloc(sizeof(struct vstack));
    s->len = 0;
    s->max_len = 8;
    s->vals = calloc(sizeof(void*), s->max_len);
    return s;
}

void vstack_push(struct vstack *s, void *v)
{
    if (s->len >= s->max_len)
    {
        /* double size of stack */
        s->max_len *= 2;
        s->vals = realloc(s->vals, sizeof(void*) * s->max_len);
    }
    s->vals[s->len] = v;
    (s->len)++;
}

void *vstack_pop(struct vstack *s)
{
    if (s->len > 0) {
        s->len--;
        return s->vals[s->len];
    }
    else
        return NULL;
}

size_t vstack_get_size(struct vstack *s)
{
    return s->len;
}

int vstack_is_empty(struct vstack *s)
{
    return s->len <= 0;
}

void vstack_destroy(struct vstack *s)
{
    free(s->vals);
    s->vals = NULL;
    s->len = 0;
    free(s);
}
