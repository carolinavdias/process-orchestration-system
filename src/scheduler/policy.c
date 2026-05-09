#include "policy.h"
#include <stddef.h>

CmdNode *pick_fcfs(UserQueue *users) {
    CmdNode   *best = NULL;
    UserQueue *u    = users;

    while (u != NULL) {
        CmdNode *cur = u->head;
        while (cur != NULL) {
            if (cur->state == CMD_WAITING) {
                if (best == NULL || cur->msg.cmd_id < best->msg.cmd_id)
                    best = cur;
                break;
            }
            cur = cur->next;
        }
        u = u->next;
    }
    return best;
}

CmdNode *pick_rr(UserQueue *users, UserQueue **rr_cursor) {
    if (users == NULL) return NULL;

    UserQueue *tmp = users;
    int found = 0;
    while (tmp != NULL) {
        if (tmp == *rr_cursor) { found = 1; break; }
        tmp = tmp->next;
    }

    if (!found) *rr_cursor = NULL;

    UserQueue *start = (*rr_cursor == NULL)
        ? users
        : ((*rr_cursor)->next != NULL ? (*rr_cursor)->next : users);

    UserQueue *cur_user = start;
    do {
        CmdNode *cur = cur_user->head;
        while (cur != NULL) {
            if (cur->state == CMD_WAITING) {
                *rr_cursor = cur_user;
                return cur;
            }
            cur = cur->next;
        }
        cur_user = (cur_user->next != NULL) ? cur_user->next : users;
    } while (cur_user != start);

    return NULL;
}