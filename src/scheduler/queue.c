#include <stdlib.h>
#include "queue.h"

UserQueue *get_user(UserQueue **users, int user_id) {
    UserQueue *u = *users;

    // procurar user existente
    while (u != NULL) {
        if (u->user_id == user_id)
            return u;
        u = u->next;
    }

    // criar novo user
    UserQueue *new_user = malloc(sizeof(UserQueue));
    if (!new_user) return NULL;

    new_user->user_id = user_id;
    new_user->head    = NULL;
    new_user->next    = NULL;

    // inserir no fim
    if (*users == NULL) {
        *users = new_user;
    } else {
        UserQueue *cur = *users;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = new_user;
    }

    return new_user;
}

void enqueue_cmd(UserQueue *u, CmdNode *node) {
    if (u->head == NULL) {
        u->head = node;
        return;
    }
    CmdNode *cur = u->head;
    while (cur->next != NULL)
        cur = cur->next;
    cur->next = node;
}

CmdNode *remove_cmd(UserQueue *u, int cmd_id) {
    CmdNode *prev = NULL;
    CmdNode *cur  = u->head;
    while (cur != NULL) {
        if (cur->msg.cmd_id == cmd_id) {
            if (prev == NULL)
                u->head = cur->next;
            else
                prev->next = cur->next;
            cur->next = NULL;
            return cur;
        }
        prev = cur;
        cur  = cur->next;
    }
    return NULL;
}

int queue_empty(UserQueue *users) {
    UserQueue *u = users;
    while (u != NULL) {
        if (u->head != NULL) return 0;
        u = u->next;
    }
    return 1;
}
