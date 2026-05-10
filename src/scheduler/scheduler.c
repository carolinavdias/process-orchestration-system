#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "scheduler.h"
#include "policy.h"

static UserQueue *users         = NULL;
static UserQueue *rr_cursor     = NULL;
static int        running_count = 0;
static int        max_parallel  = 1;
static int        next_cmd_id   = 1;
static char       policy[32]    = "";

static void free_user_if_empty(UserQueue *target) {
    if (target == NULL || target->head != NULL)
        return;

    UserQueue *prev = NULL;
    UserQueue *cur = users;
    while (cur != NULL && cur != target) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL)
        return;

    if (prev == NULL)
        users = cur->next;
    else
        prev->next = cur->next;

    if (rr_cursor == cur)
        rr_cursor = (cur->next != NULL) ? cur->next : users;

    free(cur);
}

void scheduler_init(int mp, const char *sp) {
    max_parallel  = mp;
    running_count = 0;
    next_cmd_id   = 1;
    users         = NULL;
    rr_cursor     = NULL;
    strncpy(policy, sp, sizeof(policy) - 1);
}

void scheduler_destroy(void) {
    UserQueue *u = users;
    while (u != NULL) {
        UserQueue *next_user = u->next;
        CmdNode *cur = u->head;
        while (cur != NULL) {
            CmdNode *next_cmd = cur->next;
            free(cur);
            cur = next_cmd;
        }
        free(u);
        u = next_user;
    }
    users = NULL;
    rr_cursor = NULL;
    running_count = 0;
}

int scheduler_enqueue(Message *msg) {
    CmdNode *node = malloc(sizeof(CmdNode));
    if (node == NULL) return -1;

    msg->cmd_id = next_cmd_id++;
    node->msg   = *msg;
    node->state = CMD_WAITING;
    node->next  = NULL;
    gettimeofday(&node->submit_time, NULL);  

    UserQueue *u = get_user(&users, msg->user_id);
    enqueue_cmd(u, node);

    return msg->cmd_id;
}

CmdNode *scheduler_next(void) {
    if (running_count >= max_parallel)
        return NULL;

    CmdNode *node = (strcmp(policy, "rr") == 0)
        ? pick_rr(users, &rr_cursor)
        : pick_fcfs(users);

    if (node == NULL) return NULL;

    node->state = CMD_RUNNING;
    running_count++;  
    return node;
}

long scheduler_done(int cmd_id) {
    UserQueue *u = users;
    while (u != NULL) {
        CmdNode *node = u->head;
        while (node != NULL) {
            if (node->msg.cmd_id == cmd_id && node->state == CMD_RUNNING) {
                struct timeval now;
                gettimeofday(&now, NULL);
                long ms = (now.tv_sec  - node->submit_time.tv_sec)  * 1000
                        + (now.tv_usec - node->submit_time.tv_usec) / 1000;

                running_count--;
                remove_cmd(u, cmd_id);
                free(node);
                return ms;
            }
            node = node->next;
        }
        UserQueue *next_u = u->next;
        free_user_if_empty(u);
        u = next_u;
    }
    return -1;
}

void scheduler_status(char *buf, size_t buf_len) {
    int pos = 0;

    pos += snprintf(buf + pos, buf_len - pos, "---\nExecuting\n");
    UserQueue *u = users;
    while (u != NULL) {
        CmdNode *cur = u->head;
        while (cur != NULL) {
            if (cur->state == CMD_RUNNING)
                pos += snprintf(buf + pos, buf_len - pos,
                    "user-id %d - command-id %d\n",
                    u->user_id, cur->msg.cmd_id);
            cur = cur->next;
        }
        u = u->next;
    }

    pos += snprintf(buf + pos, buf_len - pos, "---\nScheduled\n");
    u = users;
    while (u != NULL) {
        CmdNode *cur = u->head;
        while (cur != NULL) {
            if (cur->state == CMD_WAITING)
                pos += snprintf(buf + pos, buf_len - pos,
                    "user-id %d - command-id %d\n",
                    u->user_id, cur->msg.cmd_id);
            cur = cur->next;
        }
        u = u->next;
    }

    snprintf(buf + pos, buf_len - pos, "---\n");
}

int scheduler_empty(void) {
    UserQueue *u = users;
    while (u != NULL) {
        CmdNode *cur = u->head;
        while (cur != NULL) {
            if (cur->state == CMD_RUNNING) return 0;
            cur = cur->next;
        }
        u = u->next;
    }
    return 1;
}

pid_t scheduler_pop_waiting(int *cmd_id) {
    UserQueue *u = users;
    while (u != NULL) {
        CmdNode *cur = u->head;
        while (cur != NULL) {
            if (cur->state == CMD_WAITING) {
                pid_t pid = cur->msg.runner_pid;
                *cmd_id   = cur->msg.cmd_id;
                CmdNode *removed = remove_cmd(u, cur->msg.cmd_id);
                free(removed);
                return pid;
            }
            cur = cur->next;
        }
        UserQueue *next_u = u->next;
        free_user_if_empty(u);
        u = next_u;
    }
    return -1;
}