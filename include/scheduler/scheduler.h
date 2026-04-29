#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

void     scheduler_init(int max_parallel, const char *policy);
int      scheduler_enqueue(Message *msg);
CmdNode *scheduler_next(void);
long     scheduler_done(int cmd_id);
void     scheduler_status(char *buf, size_t buf_len);
int      scheduler_empty(void);

#endif
