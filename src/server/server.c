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
static void remove_server_user_by_id(int id);
static void remove_server_user_by_struct(struct server_user *su);
static size_t get_user_count();
static void write_usernames_to_dp(datapacket *dp);

// User Management
static bool is_user_logged_in(char *uname);
static struct server_user *authorize_user(SSL *ssl, char* username, char* pwd, int *res);

// Message Passing
static int send_to_client(SSL *ssl, datapacket *dp);
static int send_to_user(struct server_user *u_to, datapacket *dp);
static int send_to_all(SSL *ssl_from, datapacket *dp);

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
    sql_ch_destroy();

    return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Package Handling                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void handle_packet_auth(SSL *ssl, struct server_user *user, datapacket *dp)
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

            send_to_all(ssl, answer);

            free(msg);
        }
        break;

        case MSG_CMD_KICK_ID: {
            int tid = datapacket_get_int(dp);
            printf("received kick request: %d\n", tid);
            // check if user exists
            struct server_user *tuser = get_user_by_id(tid);
            if (tuser) {
                printf("user (%d) kicks user (%d)\n", user->id, tid);
                datapacket *answer = datapacket_create(MSG_BROADCAST);
                datapacket_set_string(answer, "admin");
                datapacket_set_string(answer, "you got kicked, fggt!");

                send_to_user(tuser, answer);

                /* kick all connections of this user */
                struct server_user_connection *c = tuser->connections;
                while(c) {
                    server_ch_disconnect_user(c->ssl, &cb_cl_dc);
                    c = c->next;
                    /* connection will be free'ed when callback is invoked */
                }
            }
        }
        break;

        case MSG_WHISPER: {
            int tid = datapacket_get_int(dp);
            struct server_user *tuser = get_user_by_id(tid);
            if (tuser) {
                char *msg = datapacket_get_string(dp);
                char name[64];
                sprintf(name, "%s(%d)", user->username, user->id);
                printf("Wsp [%d->%d]: %s\n",user->id, tid, msg);

                datapacket *answer = datapacket_create(MSG_BROADCAST);
                datapacket_set_string(answer, name);
                datapacket_set_string(answer, msg);

                send_to_user(tuser, answer);

                free(msg);
            }
            else
                printf("wsp: user not found: %d\n", tid);
        }
        break;

        case MSG_WHO: {
                int ucount = get_user_count();
                // loop all users
                // print id and name to buffer
                // send buffer to user
                datapacket *answer = datapacket_create(MSG_WHO);
                datapacket_set_int(answer, ucount);

                write_usernames_to_dp(answer);

                send_to_user(user, answer);
        }
        break;

        case MSG_LOGOUT: {
            printf("user logged out: %s\n", user->username);
            //TODO: find out which client sent it
            server_ch_user_authed(-1, ssl);
            remove_server_user_by_struct(user);
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
            /* we know that this client isn't logged in yet as it is unauthed */
            char *uname = datapacket_get_string(dp);
            char *upw = datapacket_get_string(dp);
            printf("User %s trying to login.\n", uname);

            struct server_user *user;
            int res;
            /* check auth */
            if ((user = authorize_user(ssl, uname, upw, &res))) {
                add_server_user(user);

                datapacket *answer = datapacket_create(MSG_WELCOME);
                datapacket_set_string(answer, uname);
                send_to_client(ssl, answer);
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

        case MSG_REQ_REGISTER: {
            int res;
            char *uname = datapacket_get_string(dp);
            char *upw = datapacket_get_string(dp);
            printf("User %s wants to register.\n", uname);

            if ((res = sql_ch_add_user(uname, upw)) == SQLV_SUCCESS) {
                printf("User %s sucessfuly registered.\n", uname);
                datapacket *answer = datapacket_create(MSG_REGISTR_SUCCESSFUL);
                datapacket_set_string(answer, "Registration complete. Login now.");

                size_t s = datapacket_finish(answer);
                server_ch_send(ssl, answer->data, s);

                datapacket_destroy(answer);
            }
            else {
                datapacket *answer = datapacket_create(MSG_REGISTR_FAILED);
                datapacket_set_string(answer, res == SQLV_USER_EXISTS ?
                        "Registration failed: user name already exists." :
                        "Registration failed.");

                size_t s = datapacket_finish(answer);
                server_ch_send(ssl, answer->data, s);

                datapacket_destroy(answer);

                printf("Failed to register: %d\n", res);
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
            handle_packet_auth(sc->ssl, user, dp);
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
            /* remove that connection */
            server_user_remove_connection(user, sc->ssl);

            /* if no connections left, remove user from datastructure */
            if (user->connections == NULL)
                remove_server_user_by_struct(user);
        }
        else {
            perror("dc: auth'ed client not in users list");
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * User Management                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int map_username(const gdsl_element_t e, gdsl_location_t l, void *ud)
{
    struct server_user *user = (struct server_user *)e;

    if (strcmp(user->username, (char *)ud))
        return GDSL_MAP_CONT;
    else
        return GDSL_MAP_STOP;
}

static bool is_user_logged_in(char *uname)
{
    //TODO: MySQL id lookup vs loop all users
    
    /* look for user with @uname */
    struct server_user *u = gdsl_rbtree_map_infix(users, &map_username, uname);

    return u != NULL;
}

static struct server_user *authorize_user(SSL *ssl, char *username, char* pwd, int *res)
{
    struct server_user *user = NULL;
    int uid = sql_check_user_auth(username, pwd, res);
    if (*res == SQLV_SUCCESS && uid != USER_ID_INVALID) {
        /* check if user already logged in on other machine */
        user = gdsl_rbtree_map_infix(users, &map_username, username);
        /* if already logged in, add new connection */
        if (user) {
            debugv("User already logged in, adding new connection.\n");
            server_user_add_connection(user, ssl);
        }
        /* if not yet logged in, create new server user object */
        else {
            debugv("adding new server user object.\n");
            user = server_user_create(uid, ssl, username);
        }
    
        /* inform server_ch about the user's ID */
        server_ch_user_authed(user->id, ssl);
    }

    return user;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Message Passing                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int send_to_client(SSL *ssl, datapacket *dp)
{
    size_t s = datapacket_finish(dp);
    server_ch_send(ssl, dp->data, s);
    datapacket_destroy(dp);

    return 0;
}

static int send_to_user(struct server_user *u_to, datapacket *dp)
{
    size_t s = datapacket_finish(dp);
    struct server_user_connection *p = u_to->connections;
    while (p) {
        server_ch_send(p->ssl, dp->data, s);
        p = p->next;
    }
    datapacket_destroy(dp);

    return 0;
}

static int map_send_to_all(const gdsl_element_t e, gdsl_location_t l, void *ud)
{
    struct tripplet {
        SSL *ssl;
        void *data;
        size_t data_len;
    } *d = (struct tripplet *)ud;

    int fd_from = SSL_get_fd(d->ssl);

    /* also send the message to other instances of the same user */
    struct server_user_connection *c = ((struct server_user *)e)->connections;
    while (c) {
        int fd_target = SSL_get_fd(c->ssl);
        if (fd_target != fd_from)
            server_ch_send(c->ssl, d->data, d->data_len);
        c = c->next;
    }

    return GDSL_MAP_CONT;
}

static int send_to_all(SSL *ssl_from, datapacket *dp)
{
    size_t data_len = datapacket_finish(dp);

    struct tripplet {
        SSL *ssl;
        void *data;
        size_t data_len;
    } dp_data = { ssl_from, dp->data, data_len };

    gdsl_rbtree_map_infix(users, &map_send_to_all, &dp_data);

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

static void remove_server_user_by_id(int id)
{
    struct server_user *su = get_user_by_id(id);
    if (su != NULL) {
        gdsl_rbtree_remove(users, su);
        server_user_destroy(su);
    }
}

static void remove_server_user_by_struct(struct server_user *su)
{
    if (su != NULL) {
        gdsl_rbtree_remove(users, su);
        server_user_destroy(su);
    }
}

static size_t get_user_count()
{
    return gdsl_rbtree_get_size(users);
}

static int map_write_usernames(const gdsl_element_t e, gdsl_location_t l, void *ud)
{
    struct server_user *user = (struct server_user *)e;
    char *ustr = malloc(sizeof(user->username + 64)); /* space for digits */
    sprintf(ustr, "%s(%d)", user->username, user->id);

    datapacket_set_string((datapacket *)ud, ustr);
    
    free(ustr);

    return GDSL_MAP_CONT;
}

static void write_usernames_to_dp(datapacket *dp)
{
    gdsl_rbtree_map_infix(users, &map_write_usernames, dp);
}
