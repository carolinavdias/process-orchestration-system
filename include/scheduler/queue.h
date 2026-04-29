#ifndef QUEUE_H
#define QUEUE_H

#include "utils.h"
#include <sys/time.h>

typedef enum {
    CMD_WAITING = 0,
    CMD_RUNNING = 1
} CmdState;

typedef struct CmdNode {
    Message        msg;
    CmdState       state;
    struct timeval submit_time;
    struct CmdNode *next;
} CmdNode;

typedef struct UserQueue {
    int              user_id;
    CmdNode         *head;
    struct UserQueue *next;
} UserQueue;

UserQueue *get_user(UserQueue **users, int user_id);
void       enqueue_cmd(UserQueue *u, CmdNode *node);
CmdNode   *remove_cmd(UserQueue *u, int cmd_id);
int        queue_empty(UserQueue *users);

#endif
