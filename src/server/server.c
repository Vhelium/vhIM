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

#include "../sql/sql_ch.h"
#include "server_ch.h"
#include "../network/datapacket.h"
#include "../network/messagetypes.h"
#include "server_user.h"

#define PORT "55099"

static gdsl_rbtree_t users;

// Callback Functions
static void cb_cl_cntd(struct server_client *sc);
static void cb_msg_rcv(void *sc, byte *data);
static void cb_cl_dc(struct server_client *sc);

// User Data Structure Interface
static int add_server_user(struct server_user *su);
static struct server_user *get_user_by_id(int id);
static void remove_server_user(int id);

// User Management
static struct server_user *authorize_user(SSL *ssl, char* username, char* pwd, int *res);

// Message Passing
static int send_to_user(struct server_user *u_to, datapacket *dp);
static int send_to_all(struct server_user *u_from, datapacket *dp);

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
    sql_ch_init();
    server_init();

    server_ch_start(PORT);

    server_ch_listen(&cb_cl_cntd, &cb_msg_rcv, &cb_cl_dc);

    server_ch_destroy();

    return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Package Handling                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void handle_packet_auth(struct server_user *user, datapacket *dp)
{
    int packet_type = datapacket_get_int(dp);

    switch(packet_type) {
        case MSG_BROADCAST: {
            char *msg = datapacket_get_string(dp);
            char name[64];
            sprintf(name, "%s(%d)", user->username, user->id);
            printf("Bcst [%s]: %s\n",name, msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, name);
            datapacket_set_string(answer, msg);

            send_to_all(user, answer);

            free(msg);
        }
        break;

        default:
            errv("Unknown packet(auth): %d", packet_type);
            break;
    }
}

static void handle_packet_unauth(SSL *ssl, datapacket *dp)
{
    int packet_type = datapacket_get_int(dp);

    switch(packet_type) {
        case MSG_REQ_LOGIN: {
            char *uname = datapacket_get_string(dp);
            char *upw = datapacket_get_string(dp);
            printf("User %s trying to login.\n", uname);

            struct server_user *user;
            int res;
            if ((user = authorize_user(ssl, uname, upw, &res))) {
                add_server_user(user);
                //TODO: check if already logged in

                datapacket *answer = datapacket_create(MSG_WELCOME);
                datapacket_set_string(answer, uname);
                send_to_user(user, answer);
            }
            else {
                datapacket *answer = datapacket_create(MSG_AUTH_FAILED);
                datapacket_set_string(answer, "Authentification failed.");
                datapacket_set_int(answer, res);

                size_t s = datapacket_finish(answer);
                server_ch_send(ssl, answer->data, s);

                datapacket_destroy(answer);
            }
            free(uname);
            free(upw);
        }
        break;

        default:
            errv("unauth'ed packet(%d) dropped.", packet_type);
            break;
    }
}

static void process_packet(struct server_client *sc, byte *data)
{
    datapacket *dp = datapacket_create_from_data(data);

    if (sc->id > 0) {
        struct server_user *user = get_user_by_id(sc->id);
        if (user) {
            handle_packet_auth(user, dp);
        }
        else {
            perror("pp: auth'ed client not in users list");
        }
    }
    else {
        handle_packet_unauth(sc->ssl, dp);
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Callback Functions                                          *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void cb_cl_cntd(struct server_client *sc)
{
    printf("Client connected with file descriptor #%d\n", sc_fd(sc));
}

static void cb_msg_rcv(void *sc, byte *data)
{
    debugv("Message received from client with fd #%d\n",
            sc_fd((struct server_client *)sc));
    process_packet(((struct server_client *)sc), data);
}

static void cb_cl_dc(struct server_client *sc)
{
    printf("Client disconnected with fd %d\n", sc_fd(sc));
    if (sc->id > 0) {
        struct server_user *user = get_user_by_id(sc->id);
        if (user) {
            remove_server_user(user->id);
        }
        else {
            perror("dc: auth'ed client not in users list");
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * User Management                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static struct server_user *authorize_user(SSL *ssl, char *username, char* pwd, int *res)
{
    struct server_user *user = NULL;
    if ((*res = sql_check_user_auth(username, pwd)) == SQLV_SUCCESS) {
        //TODO userid
        int id = SSL_get_fd(ssl);
        user = server_user_create(id, ssl, username);
    
        // inform server_ch about the user's ID
        server_ch_user_authed(user->id, ssl);
    }

    return user;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Message Passing                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int send_to_user(struct server_user *u_to, datapacket *dp)
{
    size_t s = datapacket_finish(dp);
    server_ch_send(u_to->ssl, dp->data, s);
    datapacket_destroy(dp);

    return 0;
}

static int map_send_to_user(const gdsl_element_t e, gdsl_location_t l, void *ud)
{
    struct tripplet {
        SSL *ssl;
        void *data;
        size_t data_len;
    } *d = (struct tripplet *)ud;

    int fd_target = SSL_get_fd(((struct server_user *)e)->ssl);
    SSL *ssl = ((struct server_user *)e)->ssl;

    int fd_from = SSL_get_fd(d->ssl);

    if (fd_target != fd_from)
        server_ch_send(ssl, d->data, d->data_len);

    return GDSL_MAP_CONT;
}

static int send_to_all(struct server_user *u_from, datapacket *dp)
{
    size_t data_len = datapacket_finish(dp);

    struct tripplet {
        SSL *ssl;
        void *data;
        size_t data_len;
    } dp_data = { u_from->ssl, dp->data, data_len };

    gdsl_rbtree_map_infix(users, &map_send_to_user, &dp_data);

    datapacket_destroy(dp);

    return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * User Data Structure Interface                               *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int add_server_user(struct server_user *su)
{
    int rc;
    gdsl_rbtree_insert(users, (void *)su, &rc);
    return rc;
}

static struct server_user *get_user_by_id(int id)
{
    return gdsl_rbtree_search(users, &compare_user_id_directly, &id);
}

static void remove_server_user(int id)
{
    struct server_user *su = get_user_by_id(id);
    if (su != NULL)
    {
        gdsl_rbtree_remove(users, su);
        server_user_destroy(su);
    }
}
