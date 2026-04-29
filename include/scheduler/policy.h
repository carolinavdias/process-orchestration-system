#ifndef POLICY_H
#define POLICY_H

#include "queue.h"

CmdNode *pick_fcfs(UserQueue *users);
CmdNode *pick_rr(UserQueue *users, UserQueue **rr_cursor);

#endif
