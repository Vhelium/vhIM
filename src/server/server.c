#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "gdsl_types.h"
#include "gdsl_rbtree.h"
#include "gdsl_list.h"

#include "../sql/sql_ch.h"
#include "server_ch.h"
#include "../network/datapacket.h"
#include "../network/messagetypes.h"
#include "server_user.h"
#include "server_group.h"
#include "../utility/vstack.h"

#define PORT "55099"

static gdsl_rbtree_t users;
static gdsl_rbtree_t active_groups;

static void gdsl_rbtree_users_free(gdsl_element_t E)
{
    server_user_destroy((struct server_user *)E);
}

static void gdsl_rbtree_active_groups_free(gdsl_element_t E)
{
    server_group_destroy((struct server_group *)E);
}

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

// User online/offline
static void on_user_online(struct server_user *su);
static void on_user_offline(struct server_user *su);

// User Management
static void on_user_added_to_group(int gid, int uid);
static void on_user_removed_from_group(int gid, int uid, bool is_owner);
static bool is_user_logged_in(char *uname);
static struct server_user *authorize_client(SSL *ssl, char* username, char* pwd, int *res);

static bool is_allowed(unsigned char priv, int cmd);

// Group Management
static void add_user_to_active_groups(struct server_user *user, int gid);
static void remove_user_from_active_groups(struct server_user *user, int gid);
static struct server_group *get_group_by_id(int id);
static void set_active_groups(datapacket *dp);
static void on_group_deleted(int gid);
static void on_group_ownership_changed(int gid, int uid_new_owner);

// Friends
static void send_friend_request(int uid_from, int uid_to);
static void accept_friend_request(int uid_from, int uid_to);
static void set_friends_online(int uid_from, datapacket *dp);
static bool users_are_friends(int uid_from, int uid_to);

// Message Passing
static int send_to_client(SSL *ssl, datapacket *dp);
static int send_to_user(struct server_user *u_to, datapacket *dp);
static int send_to_friends(struct server_user *u_from, datapacket *dp);
static int send_to_group(struct server_group *g_to, datapacket *dp);
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

// compare [server_group] with [server_group]
static long int compare_group_id(const gdsl_element_t E, void *VALUE)
{
    return ((struct server_group *)E)->id - ((struct server_group *)VALUE)->id;
}

// compare [server_group] with [group_id]
static long int compare_group_id_directly(const gdsl_element_t E, void *VALUE)
{
    return ((struct server_group *)E)->id - *((int *)VALUE);
}

static void server_init()
{
    users = gdsl_rbtree_alloc("USERS", NULL, &gdsl_rbtree_users_free, &compare_user_id);
    active_groups = gdsl_rbtree_alloc("GROUPS", NULL, &gdsl_rbtree_active_groups_free, &compare_group_id);
}

static void server_destroy()
{
    /* free the users tree */
    gdsl_rbtree_free(users);

    /* free the active_groups tree */
    gdsl_rbtree_free(active_groups);
}

int main(void)
{
    sql_ch_init();
    server_init();

    server_ch_start(PORT);

    server_ch_listen(&cb_cl_cntd, &cb_msg_rcv, &cb_cl_dc);

    server_destroy();
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
            if (is_allowed(user->p_level, MSG_CMD_KICK_ID)) {
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
                        server_ch_disconnect_client(c->ssl, &cb_cl_dc);
                        c = c->next;
                        /* connection will be free'ed when callback is invoked */
                    }
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
                // set name and id into datapacket
                datapacket *answer = datapacket_create(MSG_WHO);
                datapacket_set_int(answer, ucount);

                write_usernames_to_dp(answer);

                send_to_user(user, answer);
        }
        break;

        case MSG_FRIENDS: {
                datapacket *answer = datapacket_create(MSG_FRIENDS);
                set_friends_online(user->id, answer);

                send_to_user(user, answer);
        }
        break;

        case MSG_LOGOUT: {
            printf("user logged out: %s\n", user->username);
            /* unauth' client */
            server_ch_client_authed(-1, ssl);

            /* remove that connection from the user */
            server_user_remove_connection(user, ssl);

            /* if no connections left, remove user from datastructure */
            if (user->connections == NULL) {
                on_user_offline(user);

                /* remove user */
                remove_server_user_by_struct(user);
            }
        }
        break;

        case MSG_DISCONNECT: {
            printf("user wants to disconnect: %s\n", user->username);
            server_ch_disconnect_client(ssl, &cb_cl_dc);
            /* user get's automatically removed if no other client is connected */
        }
        break;

        case MSG_GRANT_PRIVILEGES: {
            int uid = datapacket_get_int(dp);
            int priv = datapacket_get_int(dp);
            int res = sql_ch_update_privileges(uid, priv);
            printf("Granting user with id=%d new privilege lvl: %d  (%d)\n",
                    uid, priv, res);
        }
        break;

        case MSG_ADD_FRIEND: {
            int uid_target = datapacket_get_int(dp);
            int uid_from = user->id;
            printf("user %d wants to be friend with user %d\n", uid_from, uid_target);
            if (!users_are_friends(uid_from, uid_target))
                send_friend_request(uid_from, uid_target);
            else
                printf("already friends.\n");
        }
        break;

        case MSG_REMOVE_FRIEND: {
            int uid_target = datapacket_get_int(dp);
            int uid_from = user->id;
            printf("user %d unfriends user %d\n", uid_from, uid_target);
            if (sql_ch_delete_friends(uid_from, uid_target) == SQLV_USER_EXISTS) {
                // notify users about their loss (if online)
                datapacket *answer1 = datapacket_create(MSG_REMOVE_FRIEND);
                datapacket_set_int(answer1, uid_target);
                send_to_user(get_user_by_id(uid_from), answer1);

                datapacket *answer2 = datapacket_create(MSG_REMOVE_FRIEND);
                datapacket_set_int(answer2, uid_from);
                send_to_user(get_user_by_id(uid_target), answer2);
            }
        }
        break;

        case MSG_GROUP_CREATE: {
            char *name = datapacket_get_string(dp);
            printf("[info] user %d creates group named %s\n", user->id, name);

            int gid;
            if (sql_ch_create_group(name, user->id, &gid) == SQLV_SUCCESS) {
                /* if group was created successfuly, add user to it */
                sql_ch_add_user_to_group(gid, user->id);
                on_user_added_to_group(gid, user->id);
            }

            free(name);
        }
        break;

        case MSG_GROUP_DELETE: {
            int gid = datapacket_get_int(dp);
            printf("[info] user %d removes group %d\n", user->id, gid);

            if (sql_ch_is_group_owner(gid, user->id)) {
                /* delete group in DB */
                sql_ch_delete_group(gid);
                /* clean up (e.g. active groups) */
                on_group_deleted(gid);
            }
        }
        break;

        case MSG_GROUP_ADD_USER: {
            int gid = datapacket_get_int(dp);
            int uid = datapacket_get_int(dp);
            printf("[info] user %d adds user %d to group %d\n", user->id, uid, gid);

            if (sql_ch_is_group_owner(gid, user->id)) {
                sql_ch_add_user_to_group(gid, uid);
                on_user_added_to_group(gid, uid);
            }
        }
        break;

        case MSG_GROUP_REMOVE_USER: {
            int gid = datapacket_get_int(dp);
            int uid = datapacket_get_int(dp);
            printf("[info] user %d removes user %d from group %d\n",
                    user->id, uid, gid);

            bool is_owner = sql_ch_is_group_owner(gid, user->id);
            if (uid == user->id /* self */ || is_owner /* owner */) {
                sql_ch_remove_user_from_group(gid, uid);
                on_user_removed_from_group(gid, uid, is_owner);
            }
        }
        break;
        
        case MSG_TXT_GROUP: {
            int gid = datapacket_get_int(dp);
            struct server_group *group = get_group_by_id(gid);
            if (group != NULL) {
                char *msg = datapacket_get_string(dp);
                printf("U->G [%d->%d]: %s\n",user->id, gid, msg);

                datapacket *answer = datapacket_create(MSG_TXT_GROUP);
                datapacket_set_int(answer, gid);
                datapacket_set_int(answer, user->id);
                datapacket_set_string(answer, msg);

                send_to_group(group, answer);

                free(msg);
            }
            else
                printf("gsend: group not found: %d\n", gid);
        }
        break;

        case MSG_DUMP_ACTIVE_GROUPS: {
            printf("printing active groups..\n");
            
            datapacket *answer = datapacket_create(MSG_DUMP_ACTIVE_GROUPS);
            set_active_groups(answer);
            send_to_user(user, answer);
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
            if ((user = authorize_client(ssl, uname, upw, &res))) {
                /* if not already added from other client */
                if (res != SQLV_USER_ALREADY_LOGGED_IN)
                    add_server_user(user);

                /* Welcome message */
                datapacket *answer = datapacket_create(MSG_WELCOME);
                datapacket_set_string(answer, uname);
                send_to_client(ssl, answer);

                if (user->con_len == 1) { /* just came online */
                    on_user_online(user);
                }
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

        case MSG_DISCONNECT: {
            printf("user wants to disconnect: unauth'ed\n");
            server_ch_disconnect_client(ssl, &cb_cl_dc);
            /* user get's automatically removed if no other client is connected */
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
            if (user->connections == NULL) {
                on_user_offline(user);

                /* remove user */
                remove_server_user_by_struct(user);
            }
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
    /* look for user with @uname */
    struct server_user *u = gdsl_rbtree_map_infix(users, &map_username, uname);

    return u != NULL;
}

static bool is_allowed(unsigned char priv, int cmd)
{
    switch(cmd) {
        case MSG_CMD_KICK_ID:
            return priv >= 8;
        case MSG_GRANT_PRIVILEGES:
            return priv >= 8;
        default: return false;
    }
}

static struct server_user *authorize_client(SSL *ssl, char *username, char* pwd, int *res)
{
    struct server_user *user = NULL;
    int uid = sql_ch_check_user_auth(username, pwd, res);
    if (*res == SQLV_SUCCESS && uid != USER_ID_INVALID) {
        /* check if user already logged in on other machine */
        user = gdsl_rbtree_map_infix(users, &map_username, username);
        /* if already logged in, add new connection */
        if (user) {
            debugv("User already logged in, adding new connection.\n");
            *res = SQLV_USER_ALREADY_LOGGED_IN;
            server_user_add_connection(user, ssl);
        }
        /* if not yet logged in, create new server user object */
        else {
            debugv("adding new server user object.\n");
            user = server_user_create(uid, ssl, username, sql_ch_load_privileges(uid));
        }
    
        /* inform server_ch about the user's ID */
        server_ch_client_authed(user->id, ssl);
    }

    return user;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Friends                                                     *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void send_friend_request(int uid_from, int uid_to)
{
    // check if user exists
    if (sql_ch_user_exists(uid_to)) {
        struct server_user *tuser = get_user_by_id(uid_to);
        // notify target user
        datapacket *dp = datapacket_create(MSG_SYSTEM_MSG);
        datapacket_set_string(dp, "Friend request received.");
        send_to_user(tuser, dp);
        // create db entry for request
        int res = sql_ch_create_friend_request(uid_from, uid_to);
        if (res == SQLV_OTHER_EXISTS) {
            /* other user already requested a friendship
             * ==> auto-accept */
            printf("auto accepting friend request.\n");
            sql_ch_create_friends(uid_from, uid_to); 
        }
        printf("freq code: %d\n", res);
    }
    else {
        // tell user it was an invalid name
        datapacket *dp = datapacket_create(MSG_SYSTEM_MSG);
        datapacket_set_string(dp, "User not found.");
        send_to_user(get_user_by_id(uid_from), dp);
    }
}

/* not used - instead just send a friend_request for auto-accept */
static void accept_friend_request(int uid_from, int uid_to)
{
   sql_ch_create_friends(uid_from, uid_to); 
}

static void set_friends_online(int uid_from, datapacket *dp)
{
    // on = #friends online
    // off = #friends offline
    struct vstack *friends, *off, *on, *requests;
    friends = vstack_create();
    off = vstack_create();
    on = vstack_create();
    requests = vstack_create();
    // retrieve list of friends from sql_ch
    sql_ch_get_friends(uid_from, friends);
    // loop list and see who is online/offline
    while (!vstack_is_empty(friends)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(friends);
        struct server_user *u = get_user_by_id(info->id);
        if (u != NULL) {
            vstack_push(on, info);
        }
        else {
            vstack_push(off, info);
        }
    }
    // fill datapacket correspondingly
    size_t on_len = vstack_get_size(on);
    datapacket_set_int(dp, on_len);
    int i;
    for (i = 0; i < on_len; ++i) {
        struct server_user_info *info = vstack_pop(on);
        datapacket_set_int(dp, info->id);
        datapacket_set_string(dp, info->username);
        free(info->username);
        free(info);
    }

    size_t off_len = vstack_get_size(off);
    datapacket_set_int(dp, off_len);
    for(i = 0; i < off_len; ++i) {
        struct server_user_info *info = vstack_pop(off);
        datapacket_set_int(dp, info->id);
        datapacket_set_string(dp, info->username);
        free(info->username);
        free(info);
    }
    
    // retrieve list of pending friend requests
    sql_ch_get_friend_requests(uid_from, requests);
    // set size of pending requests
    datapacket_set_int(dp, vstack_get_size(requests));
    // loop list and see who is in it
    while (!vstack_is_empty(requests)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(requests);
        datapacket_set_int(dp, info->id);
        datapacket_set_string(dp, info->username);
        free(info->username);
        free(info);
    }

    vstack_destroy(friends);
    vstack_destroy(off);
    vstack_destroy(on);
    vstack_destroy(requests);
}

static bool users_are_friends(int uid_from, int uid_to)
{
    return sql_ch_are_friends(uid_from, uid_to);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Message Passing                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int send_to_client(SSL *ssl, datapacket *dp)
{
    if (ssl == NULL) {
        datapacket_destroy(dp);
        return 2;
    }
    size_t s = datapacket_finish(dp);
    server_ch_send(ssl, dp->data, s);
    datapacket_destroy(dp);

    return 0;
}

static int send_data_to_user(struct server_user *u_to, byte *data, size_t s)
{
    if (u_to == NULL)
        return 2;

    struct server_user_connection *p = u_to->connections;
    while (p) {
        server_ch_send(p->ssl, data, s);
        p = p->next;
    }
    return 0;
}

static int send_to_user(struct server_user *u_to, datapacket *dp)
{
    if (u_to == NULL) {
        datapacket_destroy(dp);
        return 2;
    }
    size_t s = datapacket_finish(dp);
    send_data_to_user(u_to, dp->data, s);
    datapacket_destroy(dp);

    return 0;
}

static int send_to_friends(struct server_user *u_from, datapacket *dp)
{
    size_t s = datapacket_finish(dp);

    struct vstack *friends;
    friends = vstack_create();
    // retrieve list of friends from sql_ch
    sql_ch_get_friends(u_from->id, friends);
    // loop list and see who is online/offline
    while (!vstack_is_empty(friends)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(friends);
        struct server_user *u = get_user_by_id(info->id);
        if (u != NULL) {
            send_data_to_user(u, dp->data, s);
        }
        free(info->username);
        free(info);
    }
    /* clean up */
    vstack_destroy(friends);

    datapacket_destroy(dp);
    return 0;
}

static int send_to_group(struct server_group *g_to, datapacket *dp)
{
    size_t s = datapacket_finish(dp);

    gdsl_element_t e;
    gdsl_list_cursor_t c = gdsl_list_cursor_alloc(g_to->members);
    
    for (gdsl_list_cursor_move_to_head(c); (e = gdsl_list_cursor_get_content(c)); gdsl_list_cursor_step_forward(c)) {
        int uid = *((int *)e);
        struct server_user *user = get_user_by_id(uid);
        if (user == NULL) {
            /* should NOT happen.
             * Only ONLINE users must by in members of an active group! */
            printf("[EE] USER %d = NULL IN SEND_TO_GROUP with gid=%d\n", uid, g_to->id);
        }
        else {
            send_data_to_user(user, dp->data, s);
        }
    }

    gdsl_list_cursor_free(c);
    
    datapacket_destroy(dp);
    return 0;
}

struct sender_triplet {
        SSL *ssl;
        void *data;
        size_t data_len;
};

static int map_send_to_all(const gdsl_element_t e, gdsl_location_t l, void *ud)
{
    struct sender_triplet *d = (struct sender_triplet*)ud;

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

    struct sender_triplet dp_data = { ssl_from, dp->data, data_len };

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

    datapacket_set_int((datapacket *)ud, user->id); /* id */
    datapacket_set_string((datapacket *)ud, user->username);  /* name */

    return GDSL_MAP_CONT;
}

static void write_usernames_to_dp(datapacket *dp)
{
    gdsl_rbtree_map_infix(users, &map_write_usernames, dp);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * User Online Offline                                         *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void on_user_online(struct server_user *user)
{
    /* inform friends */
    datapacket *on = datapacket_create(MSG_FRIEND_ONLINE);
    datapacket_set_int(on, user->id);
    datapacket_set_string(on, user->username);
    send_to_friends(user, on);

    /* fetch all groups from user */
    struct vistack *user_groups;
    sql_ch_get_groups_of_user(user->id, &user_groups);

    while (!vistack_is_empty(user_groups)) {
        int gid = vistack_pop(user_groups);
        add_user_to_active_groups(user, gid);
    }
    vistack_destroy(user_groups);
}

static void on_user_offline(struct server_user *user)
{
    /* inform friends */
    datapacket *off = datapacket_create(MSG_FRIEND_OFFLINE);
    datapacket_set_int(off, user->id);
    send_to_friends(user, off);

    /* update groups */
    struct vistack *user_groups;
    sql_ch_get_groups_of_user(user->id, &user_groups);

    while (!vistack_is_empty(user_groups)) {
        int gid = vistack_pop(user_groups);
        remove_user_from_active_groups(user, gid);
    }
    vistack_destroy(user_groups);
}

static void on_user_added_to_group(int gid, int uid)
{
    struct server_user *user = get_user_by_id(uid);
    if (user != NULL) { /* user is online */
        add_user_to_active_groups(user, gid);
    }
}

static void on_user_removed_from_group(int gid, int uid, bool is_owner)
{
    /* check if anyone is left in that group */
    int mem_count;
    sql_ch_get_member_count_of_group(gid, &mem_count);
    if (mem_count <= 0) {
        /* delete group in DB */
        sql_ch_delete_group(gid);
        /* clean up (e.g. active groups) */
        on_group_deleted(gid);
    }
    /* check if removed user was owner */
    else if (is_owner) {
        /* pass ownership */
        int uid_new_owner;
        sql_ch_pass_group_ownership(gid, uid, &uid_new_owner);
        on_group_ownership_changed(gid, uid_new_owner);
    }

    struct server_user *user = get_user_by_id(uid);
    if (user != NULL) { /* user is online */
        remove_user_from_active_groups(user, gid);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Group Management                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static struct server_group *get_group_by_id(int gid)
{
    return gdsl_rbtree_search(active_groups, &compare_group_id_directly, &gid);
}

/* adds user to the (locally) active group with ID `gid`
 * will initialize the group if not existing (locally) yet */
static void add_user_to_active_groups(struct server_user *user, int gid)
{
    struct server_group *group = get_group_by_id(gid);
    /*
     * 1: Group not active yet
     */
    if (group == NULL) {
        debugv("adding user(%d) to active group(%d): group=NULL\n", user->id, gid);
        /* create new group object */
        group = server_group_create();
        /* fetch group infos from DB */
        sql_ch_initialize_group(gid, group);
        /* add user to the group object */
        server_group_add_user(group, user->id);
        /* add group to active_groups */
        int res;
        gdsl_rbtree_insert(active_groups, group, &res);
    }
    /*
     * 2: Group already active
     */
    else {
        debugv("adding user(%d) to active group(%d): group=existing\n", user->id, gid);
        /* add user to the group object */
        server_group_add_user(group, user->id);
    }
}

/* removes user from active group
 * should only happen if user goes offline or gets removed from group */
static void remove_user_from_active_groups(struct server_user *user, int gid)
{
    struct server_group *group = get_group_by_id(gid);

    if (group != NULL) {
        debugv("removing user(%d) to active group(%d): group=NULL\n", user->id, gid);
        server_group_remove_user(group, user->id);
        
        /* check if user was the last online member in active group */
        if (server_group_member_count(group) <= 0) {
            /* remove group from active groups and free the object */
            gdsl_rbtree_remove(active_groups, group);
            server_group_destroy(group);
        }
    }
    else {
        /* it should not be NULL.. each group an active user is in should be here! */
        printf("[EE] remove user(%d) from active group: group(%d) does not exist!\n", user->id, gid);
    }
}

static int map_write_active_groups(const gdsl_element_t el, gdsl_location_t l, void *ud)
{
    struct server_group *grp = (struct server_group *)el;
    datapacket *dp = (datapacket *)ud;

    datapacket_set_int(dp, grp->id); /* id */
    datapacket_set_string(dp, grp->name);  /* name */
    datapacket_set_int(dp, grp->owner_id); /* owner id */
    datapacket_set_int(dp, server_group_member_count(grp)); /* set member count */
    
    /* loop over members */
    gdsl_element_t e;
    gdsl_list_cursor_t c = gdsl_list_cursor_alloc(grp->members);
    
    for (gdsl_list_cursor_move_to_head(c); (e = gdsl_list_cursor_get_content(c)); gdsl_list_cursor_step_forward(c)) {
        int uid = *((int *)e);
        datapacket_set_int(dp, uid);
    }

    gdsl_list_cursor_free(c);

    return GDSL_MAP_CONT;
}

static void write_active_groups_to_dp(datapacket *dp)
{
    gdsl_rbtree_map_infix(active_groups, &map_write_active_groups, dp);
}

static void set_active_groups(datapacket *dp)
{
    datapacket_set_int(dp, (int)gdsl_rbtree_get_size(active_groups));
    write_active_groups_to_dp(dp);
}

/* group permanently deleted (not just the active group!) */
static void on_group_deleted(int gid)
{
    struct server_group *group = get_group_by_id(gid);
    if (group != NULL) {
        /* send message to online users in that group */
        datapacket *dp = datapacket_create(MSG_GROUP_DELETE);
        datapacket_set_int(dp, gid);
        send_to_group(group, dp);

        /* remove group from active groups */
        gdsl_rbtree_remove(active_groups, group);
        server_group_destroy(group);
    }
}

static void on_group_ownership_changed(int gid, int uid_new_owner)
{
    struct server_group *sg = get_group_by_id(gid);
    if (sg != NULL) {
        sg->owner_id = uid_new_owner; 

        /* send message to online users in that group */
        datapacket *dp = datapacket_create(MSG_GROUP_OWNER_CHANGED);
        datapacket_set_int(dp, gid);
        datapacket_set_int(dp, uid_new_owner);
        send_to_group(sg, dp);
    }
}
