#ifndef SERVER_GROUP_H
#define SERVER_GROUP_H

#include "gdsl_list.h"

struct server_group
{
    int id;
    char *name;
    int owner_id;
    gdsl_list_t members;
};

/* new memory will be allocated for username and the linked list for connections */
struct server_group *server_group_create();

/* initialize group */
void server_group_initialize(struct server_group *group, int id, char *name, int owner_id);

/* add user id to the members of the group */
void server_group_add_user(struct server_group *sg, int uid);

/* remove user id from members */
void server_group_remove_user(struct server_group *sg, int uid);

/* returns count of members stack */
int server_group_member_count(struct server_group *sg);

/* has to be invoked manually */
void server_group_destroy(struct server_group *sg);

#endif
