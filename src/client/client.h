#ifndef CLIENT_H
#define CLIENT_H

bool cl_get_is_connected_synced();

int cl_exec_broadcast(const char *msg);

int cl_exec_kick(int uid);

int cl_exec_whisper(int uid, const char *msg);

int cl_exec_req_register(const char *name, const char *pw);

int cl_exec_req_login(const char *name, const char *pw);

int cl_exec_logout(void);

int cl_exec_who(void);

int cl_exec_friends(void);

int cl_exec_connect(const char *server, int port);

int cl_exec_disconnect(void);

int cl_exec_grant_privileges(int uid, int lvl);

int cl_exec_add_friend(int uid);

int cl_exec_remove_friend(int uid);

int cl_exec_group_create(const char *name);

int cl_exec_group_delete(int gid);

int cl_exec_group_add_user(int gid, int uid);

int cl_exec_group_send(int gid, const char *msg);

int cl_exec_group_dump_active(void);

#endif
