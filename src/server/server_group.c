#include <stdlib.h>
#include <string.h>
#include "server_group.h"

#include "../utility/vistack.h"
#include "../constants.h"
#include "openssl/ssl.h"

struct server_group *server_group_create(int id, char *name, int owner_id)
{
    struct server_group *group = malloc(sizeof(struct server_group));
    group->id = id;
    group->name = strdup(name);
    group->owner_id = owner_id;
    group->members = NULL;
    return group;
}

void server_group_destroy(struct server_group *sg)
{
    free(sg->name);
    sg->name = NULL;
    free(sg);
}
