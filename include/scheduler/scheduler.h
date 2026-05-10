#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

void     scheduler_init(int max_parallel, const char *policy);
void     scheduler_destroy();
int      scheduler_enqueue(Message *msg);
CmdNode *scheduler_next();
long     scheduler_done(int cmd_id);
void     scheduler_status(char *buf, size_t buf_len);
int      scheduler_empty();

#endif
