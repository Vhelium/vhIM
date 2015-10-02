#ifndef SERVER_USER_H
#define SERVER_USER_H

struct server_user
{
    int fd;
    int id;
    char *username;
};

// new memory will be allocated for username and pwd
struct server_user *server_user_create(int id, int fd, char *username);

// destructor gets 'automatically' called by RB-Tree. Do not invoke manually!
void server_user_destroy(struct server_user *su);

#endif
