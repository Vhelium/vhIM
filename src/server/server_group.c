#include <stdlib.h>
#include <string.h>
#include "server_group.h"

#include "gdsl_list.h"
#include "../constants.h"
#include "openssl/ssl.h"

static gdsl_element_t member_alloc(void *data)
{
    int *i = calloc(1, sizeof(int));
    *i = *((int *)data);
    return i;
}

static void member_free(gdsl_element_t e)
{
    free((int *)e);
}

static long int member_compare(const gdsl_element_t E, void *VALUE)
{
    return *((int *)E) - *((int *)VALUE);
}

struct server_group *server_group_create()
{
    struct server_group *group = malloc(sizeof(struct server_group));
    group->id = -1;
    group->name = NULL;
    group->owner_id = -1;
    group->members = gdsl_list_alloc("SERVER_GROUP_MEMBERS", &member_alloc, &member_free);
    return group;
}

void server_group_initialize(struct server_group *group, int id, char *name, int owner_id)
{
    group->id = id;
    group->name = strdup(name);
    group->owner_id = owner_id;
}

void server_group_add_user(struct server_group *sg, int uid)
{
    gdsl_list_insert_head(sg->members, &uid); 
}

void server_group_remove_user(struct server_group *sg, int uid)
{
    gdsl_list_delete(sg->members, &member_compare, &uid); 
}

int server_group_member_count(struct server_group *sg)
{
    return gdsl_list_get_size(sg->members);
}

void server_group_destroy(struct server_group *sg)
{
    free(sg->name);
    sg->name = NULL;
    gdsl_list_free(sg->members);
    sg->members = NULL;
    free(sg);
}
