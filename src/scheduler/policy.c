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

    if (*rr_cursor == NULL)
        *rr_cursor = users;

    UserQueue *start = *rr_cursor;

    do {
        CmdNode *cur = (*rr_cursor)->head;
        while (cur != NULL) {
            if (cur->state == CMD_WAITING) {
                *rr_cursor = ((*rr_cursor)->next != NULL) ? (*rr_cursor)->next : users;
                return cur;
            }
            cur = cur->next;
        }
        *rr_cursor = ((*rr_cursor)->next != NULL) ? (*rr_cursor)->next : users;
    } while (*rr_cursor != start);

    return NULL;
}
