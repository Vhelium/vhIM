#ifndef SERVER_GROUP_H
#define SERVER_GROUP_H

#include "../utility/vistack.h"

struct server_group
{
    int id;
    char *name;
    int owner_id;
    struct vistack *members;
};

/* new memory will be allocated for username and the linked list for connections */
struct server_group *server_group_create(int id, char *name, int owner_id);

/* has to be invoked manually */
void server_group_destroy(struct server_group *sg);

#endif
