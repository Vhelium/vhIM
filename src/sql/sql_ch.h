#ifndef SQL_CH_H
#define SQL_CH_H

#include <stdbool.h>
#include "../utility/vstack.h"
#include "../utility/vistack.h"
#include "../server/server_group.h"

#define SQLV_SUCCESS 0
#define SQLV_FAILURE 1
#define SQLV_USER_NOT_FOUND 2
#define SQLV_WRONG_PASSWORD 3
#define SQLV_CONNECTION_ERROR 4
#define SQLV_USER_EXISTS 5
#define SQLV_USER_ALREADY_LOGGED_IN 6
#define SQLV_ENTRY_EXISTS 7
#define SQLV_NOPE 8
#define SQLV_OTHER_EXISTS 9

void sql_ch_init();

/*
 * Function: sql_ch_check_user_auth
 * ----------------------------
 *   Validates user credentials
 *
 *   user: username
 *   pw: password
 *   res: adress where the result code will be stored
 *
 *   returns: userID if auth successful, USER_ID_INVALID otherwise
 */
int sql_ch_check_user_auth(char *user, char *pw, int *res);

/*
 * Function: sql_ch_add_user
 * ----------------------------
 *   Creates new user entry in the database.
 *
 *   user: username
 *   pw: password
 *
 *   returns: SQLV status code
 *              SQLV_SUCCESS: user sucessfully created
 *              SQLV_CONNECTION_ERROR: error connecting to the database
 *              SQLV_USER_EXISTS: user already exists
 */
int sql_ch_add_user(char *user, char *pw);

/*
 * Function: sql_ch_load_privileges
 * ----------------------------
 *   Loads a user's privilege level from the database.
 *
 *   uid: user
 *
 *   returns: privilege level of user `uid`
 */
unsigned char sql_ch_load_privileges(int uid);

/*
 * Function: sql_ch_update_privileges
 * ----------------------------
 *   Modifies the privilege level of a user
 *
 *   uid: user
 *   p: new privilege level
 *
 *   returns: 0 on success, > 0 otherwise
 */
int sql_ch_update_privileges(int uid, unsigned char p);

/*
 * Function: sql_ch_create_friend_request
 * ----------------------------
 *   Creates new friend request (checks for duplicate) and auto-accepts
 *   pending requests from `uid_to` to `uid_from` which results in
 *   a fix friendship between `uid_from` and `uid_to`
 *
 *   uid_from: requesting user
 *   uid_to: target user
 *
 *   returns: 1 if uid_from and uid_to are friends, 0 otherwise 
 */
int sql_ch_create_friend_request(int uid_from, int uid_to);

/*
 * Function: sql_ch_get_friend_requests
 * ----------------------------
 *   Looks up pending friend requests for specified user
 *
 *   uid_from: users who's incoming requests we check
 *   requests_out: adress to stack where info will be stored 
 *
 *   returns: SQLV status code
 */
int sql_ch_get_friend_requests(int uid_from, struct vstack *requests_out);

/*
 * Function: sql_ch_are_friends
 * ----------------------------
 *   Checks if two users are friends.
 *
 *   uid_from: user id 1
 *   uid_to: user id 2
 *
 *   returns: 1 if uid_from and uid_to are friends, 0 otherwise 
 */
bool sql_ch_are_friends(int uid_from, int uid_to);

/*
 * Function: sql_ch_create_friends
 * ----------------------------
 *   Marks two user as friends.
 *   Will NOT check for an existing friendship.
 *   Will delete corresponding friend_requests.
 *
 *   uid_1: user id 1 
 *   uid_2: user id 2
 *
 *   returns: SQLV status code
 *              SQLV_SUCCESS: friendship sucessfully created
 *              SQLV_CONNECTION_ERROR: error connecting to the database
 */
int sql_ch_create_friends(int uid_1, int uid_2);

/*
 * Function: sql_ch_get_friends
 * ----------------------------
 *   Looks up all friends of a user with id `uid_from`
 *   and pushes them into provided stack `friends_out`.
 *
 *   uid_from: target user
 *   friends_out: pointer to output stack
 *
 *   returns: SQLV status code
 *              SQLV_SUCCESS: friends sucessfully written to the stack 
 *              SQLV_CONNECTION_ERROR: error connecting to the database
 */
int sql_ch_get_friends(int uid_from, struct vstack *friends_out);

/*
 * Function: sql_ch_delete_friends
 * ----------------------------
 *   Removes the friendship between two users and any pending requests
 *
 *   uid_from: user id 1
 *   uid_to: user id 2
 *
 *   returns: SQLV status code
 */
int sql_ch_delete_friends(int uid_1, int uid_2);

/* returns if user with id `uid` exists */
bool sql_ch_user_exists(int uid);

/* returns the count of members in the given group */
int sql_ch_get_member_count_of_group(int gid);

/* creates new group with name `name` and owner `uid_owner` */
int sql_ch_create_group(const char *name, int uid_owner, int *gid);

/* adds user to group, if not already in it.
 * will NOT check if user is allowed to do so. check beforehand! */
int sql_ch_add_user_to_group(int gid, int uid);

/* Removes user from group
 * Does not do any cleanup of the group itself */
int sql_ch_remove_user_from_group(int gid, int uid);

/* Deletes group
 * Will delete any user<->group relations */
int sql_ch_delete_group(int gid);

/* Passes owner ship to next user (the one first added) */
int sql_ch_pass_group_ownership(int gid, int uid_old_owner);

/* returns true if user with id `uid` is owner of group `gid` */
bool sql_ch_is_group_owner(int gid, int uid);

/* returns a stack with all IDs of groups the user is assigned to */
int sql_ch_get_groups_of_user(int uid, struct vistack **g);

/* fetches group info and fills it into group struct */
int sql_ch_initialize_group(int gid, struct server_group *grp);

/* clean up */
void sql_ch_destroy();

#endif
