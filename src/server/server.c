#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "gdsl_types.h"
#include "gdsl_rbtree.h"

#include "server_ch.h"
#include "../network/datapacket.h"
#include "../network/messagetypes.h"
#include "server_user.h"

#define PORT "55099"

static gdsl_rbtree_t users;
static struct server_user *users_cnt;

// Callback Functions
static void cb_cl_cntd(int fd);
static void cb_msg_rcv(int fd, byte *data);
static void cb_cl_dc(int fd);

// User Data Structure Interface
static int add_server_user(struct server_user *su);
static struct server_user *search_server_user(int id);
static void remove_server_user(int id);

// User Management
static struct server_user *authorize_user(int fd, char* username, char* pwd);

// Message Passing
static int send_to_user(int id, byte *data);
static int send_to_all(byte *data);

// compare [server_user] with [server_user]
static long int compare_user_id(const gdsl_element_t E, void *VALUE)
{
    return ((struct server_user *)E)->id - ((struct server_user *)VALUE)->id;
}

// compare [server_user] with [user_id]
static long int compare_user_id_directly(const gdsl_element_t E, void *VALUE)
{
    return ((struct server_user *)E)->id - *((int *)VALUE);
}

static void server_init()
{
    users = gdsl_rbtree_alloc("USERS", NULL, NULL, &compare_user_id);
}

int main(void)
{
    server_init();

    server_ch_start(PORT);

    server_ch_listen(&cb_cl_cntd, &cb_msg_rcv, &cb_cl_dc);

    server_ch_destroy();

    return 0;
}

static void process_packet(int fd, byte *data)
{
    datapacket *dp = datapacket_create_from_data(data);
    int packet_type = datapacket_get_int(dp);

    char *uname;

    switch(packet_type) {
        case MSG_LOGIN:
            uname = datapacket_get_string(dp);
            printf("Login as: %s", uname);

            struct server_user *user;
            if (user = authorize_user(fd, uname, NULL)) {
                int r = add_server_user(user);

                datapacket *answer = datapacket_create(MSG_WELCOME);
                datapacket_set_string(answer, uname);
                size_t s = datapacket_finish(answer);
                server_ch_send(fd, answer->data, s);
            }
            else {
                datapacket *answer = datapacket_create(MSG_AUTH_FAILED);
                datapacket_set_string(answer, "Authentification failed.");
                size_t s = datapacket_finish(answer);
                server_ch_send(fd, answer->data, s);
            }
            free(uname);
        break;

        case MSG_BROADCAST:
        {
            char *msg = datapacket_get_string(dp);
            // TODO: get userid from fd..
            char name[32];
            sprintf(name, "User%d", fd);
            printf("Bcst [%s]: %s\n",name, msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, name);
            datapacket_set_string(answer, msg);
            size_t s = datapacket_finish(answer);
            server_ch_send_all(fd, answer->data, s);

            free(msg);
        }
        break;

        default:
            printf("Unknown packet: %d", packet_type);
            break;
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

/*
 * Callback Functions
 */
static void cb_cl_cntd(int fd)
{
    printf("Client connected with file descriptor #%d\n", fd);
}

static void cb_msg_rcv(int fd, byte *data)
{
    printf("Message received from client with fd #%d\n", fd);
    process_packet(fd, data);
}

static void cb_cl_dc(int fd)
{
    printf("Client disconnected with file descriptor #%d\n", fd);
}

/*
 * User Management
 */
static int get_fd_for_user(int user_id)
{
    struct server_user *su = search_server_user(user_id);
    return su->fd;
}

static struct server_user *authorize_user(int fd, char *username, char* pwd)
{
    struct server_user *user = NULL;
    if (true) {
        user = server_user_create(1, fd, username);
    }
    //TODO
    return user;
}

/*
 * Message Passing
 */
static int send_to_user(int id, byte *data)
{
    
}

static int send_to_all(byte *data)
{

}

/*
 * User Data Structure Interface
 */
static int add_server_user(struct server_user *su)
{
    int rc;
    gdsl_rbtree_insert(users, (void *)su, &rc);
    return rc;
}

static struct server_user *search_server_user(int id)
{
    return gdsl_rbtree_search(users, &compare_user_id_directly, &id);
}

static void remove_server_user(int id)
{
    struct server_user *su = search_server_user(id);
    if (su != NULL)
    {
        gdsl_rbtree_remove(users, su);
        server_user_destroy(su);
    }
}
