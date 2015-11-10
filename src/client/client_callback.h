#ifndef CLIENT_CALLBACK_H
#define CLIENT_CALLBACK_H

#include "../utility/vstack.h"

typedef int (*cb_generic_t)(void);

typedef void (*cb_welcome_t)(const char *msg);
typedef void (*cb_auth_failed_t)(const char *msg, int res);
typedef void (*cb_broadcast_t)(const char *name, const char *msg);
typedef void (*cb_system_msg_t)(const char *msg);
typedef void (*cb_registr_successful_t)(const char *msg);
typedef void (*cb_registr_failed_t)(const char *msg);
typedef void (*cb_who_t)(struct vstack *users);
typedef void (*cb_friends_t)(struct vstack *on, struct vstack *off, struct vstack *req);
typedef void (*cb_remove_friend_t)(int uid);
typedef void (*cb_friend_online_t)(int uid, const char *name);
typedef void (*cb_friend_offline_t)(int uid);
typedef void (*cb_group_dump_active_t)(const char *msg);

#endif
