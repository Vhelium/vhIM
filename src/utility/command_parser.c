#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../network/messagetypes.h"
#include "../constants.h"
#include "command_parser.h"
#include "strings_helper.h"

int process_command(char *input_buffer, int (*exec_cmd)(int, char**))
{
    int ret = -1;
    char *cmd = input_buffer + 1;
    char *type = NULL;
    if (!next_word(&cmd, &type)) { /* get first argument */
        return 4;
    }

    /* kick */
    if (strcmp(type, "kick") == 0) {
        char *user = NULL;
        if (next_word(&cmd, &user)) {
            char *a[] = {user};
            exec_cmd(MSG_CMD_KICK_ID, a);
        }
        else
            ret = 2;
        free(user);
    }
    /* whisper */
    else if (strcmp(type, "w") == 0) {
        char *user = NULL;
        if (next_word(&cmd, &user)) {
            char *txt = NULL;
            if (next_word(&cmd, &txt)) {
                char *a[] = {user, txt};
                exec_cmd(MSG_WHISPER, a);
            }
            else
                ret = 2;
            free(txt);
        }
        else
            ret = 2;
        free(user);
    }
    /* registration */
    else if (strcmp(type, "register") == 0) {
        char *uname = NULL;
        if (next_word(&cmd, &uname)) {
            char *upw = NULL;
            if( next_word(&cmd, &upw)) {
                char *a[] = {uname, upw};
                exec_cmd(MSG_REQ_REGISTER, a);
            }
            else
                ret = 2;
            free(upw);
        }
        else
            ret = 2;
        free (uname);
    }
    /* login */
    else if (strcmp(type, "login") == 0) {
        char *uname = NULL;
        if (next_word(&cmd, &uname)) {
            char *upw = NULL;
            if( next_word(&cmd, &upw)) {
                char *a[] = {uname, upw};
                exec_cmd(MSG_REQ_LOGIN, a);
            }
            else
                ret = 2;
            free(upw);
        }
        else
            ret = 2;
        free (uname);
    }
    /* logout */
    else if (strcmp(type, "logout") == 0) {
        exec_cmd(MSG_LOGOUT, NULL);
    }
    /* who */
    else if (strcmp(type, "who") == 0) {
        exec_cmd(MSG_WHO, NULL);
    }
    /* friends*/
    else if (strcmp(type, "friends") == 0) {
        exec_cmd(MSG_FRIENDS, NULL);
    }
    /* help */
    else if (strcmp(type, "help") == 0) {
        exec_cmd(CMD_HELP, NULL);
    }
     /* connect */
    else if (strcmp(type, "connect") == 0) {
        char *host = NULL;
        char *port = NULL;
        if (next_word(&cmd, &host)) {
            next_word(&cmd, &port);
        }
        char *a[] = {host, port};
        exec_cmd(CMD_CONNECT, a);

        if (host) {
            free (host);
        }
        if (port) {
            free (port);
        }
    }
    /* disconnect */
    else if (strcmp(type, "disconnect") == 0 || strcmp(type, "dc") == 0) {
        exec_cmd(CMD_DISCONNECT, NULL);
    }
    /* grant privileges*/
    else if (strcmp(type, "grant") == 0) {
        char *uid;
        if (next_word(&cmd, &uid)) {
            char *lvl;
            if( next_word(&cmd, &lvl)) {
                char *a[] = {uid, lvl};
                exec_cmd(MSG_GRANT_PRIVILEGES, a);
            }
            else
                ret = 2;
            free(lvl);
        }
        else
            ret = 2;
        free (uid);
    }
    /* friend add */
    else if (strcmp(type, "fadd") == 0) {
        char *user = NULL;
        if (next_word(&cmd, &user)) {
            char *a[] = {user};
            exec_cmd(MSG_ADD_FRIEND, a);
        }
        else
            ret = 2;
        free(user);
    }
    /* friend remove */
    else if (strcmp(type, "fremove") == 0) {
        char *user = NULL;
        if (next_word(&cmd, &user)) {
            char *a[] = {user};
            exec_cmd(MSG_REMOVE_FRIEND, a);
        }
        else
            ret = 2;
        free(user);
    }
    /* group create */
    else if (strcmp(type, "gcreate") == 0) {
        char *name = NULL;
        if (next_word(&cmd, &name)) {
            char *a[] = {name};
            exec_cmd(MSG_GROUP_CREATE, a);
        }
        else
            ret = 2;
        free(name);
    }
    /* group add user */
    else if (strcmp(type, "gadd") == 0) {
        char *gid;
        if (next_word(&cmd, &gid)) {
            char *uid;
            if(next_word(&cmd, &uid)) {
                char *a[] = {gid, uid};
                exec_cmd(MSG_GROUP_ADD_USER, a);
            }
            else
                ret = 2;
            free(uid);
        }
        else
            ret = 2;
        free (gid);
    }
    /* group delete */
    else if (strcmp(type, "gdelete") == 0) {
        char *gid= NULL;
        if (next_word(&cmd, &gid)) {
            char *a[] = {gid};
            exec_cmd(MSG_GROUP_DELETE, a);
        }
        else
            ret = 2;
        free(gid);
    }
    /* group send */
    else if (strcmp(type, "g") == 0) {
        char *group = NULL;
        if (next_word(&cmd, &group)) {
            char *txt = NULL;
            if (next_word(&cmd, &txt)) {
                char *a[] = {group, txt};
                exec_cmd(MSG_GROUP_SEND, a);
            }
            else
                ret = 2;
            free(txt);
        }
        else
            ret = 2;
        free(group);
    }
    /* active groups dump*/
    else if (strcmp(type, "gadump") == 0) {
        exec_cmd(MSG_DUMP_ACTIVE_GROUPS, NULL);
    }
    /* invalid command */
    else
        ret = 2;

    if (ret == 2)
        exec_cmd(CMD_INVALID, NULL);
    
    free(type);

    return ret;
}
