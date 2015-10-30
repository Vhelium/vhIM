#include "vistack.h"
#include <stdlib.h>

struct vistack *vistack_create()
{
    struct vistack *s = malloc(sizeof(struct vistack));
    s->len = 0;
    s->max_len = 8;
    s->vals = calloc(sizeof(int), s->max_len);
    return s;
}

void vistack_push(struct vistack *s, int v)
{
    if (s->len >= s->max_len)
    {
        /* double size of stack */
        s->max_len *= 2;
        s->vals = realloc(s->vals, sizeof(int) * s->max_len);
    }
    s->vals[s->len] = v;
    (s->len)++;
}

int vistack_pop(struct vistack *s)
{
    if (s->len > 0) {
        s->len--;
        return s->vals[s->len];
    }
    else
        return -1;
}

size_t vistack_get_size(struct vistack *s)
{
    return s->len;
}

int vistack_is_empty(struct vistack *s)
{
    return s->len <= 0;
}

void vistack_destroy(struct vistack *s)
{
    free(s->vals);
    s->vals = NULL;
    s->len = 0;
    free(s);
}
