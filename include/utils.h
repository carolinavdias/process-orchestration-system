#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define CONTROLLER_FIFO "/tmp/controller_fifo"
#define RUNNER_FIFO_FMT "/tmp/runner_fifo_%d"
#define RUNNER_FIFO_LEN  64
 
#define MAX_CMD_LEN  2048
#define MAX_ARGS     32

typedef enum {
    MSG_EXEC     = 1,
    MSG_DONE     = 2,
    MSG_STATUS   = 3,
    MSG_SHUTDOWN = 4
} MsgType;
 
typedef struct {
    MsgType type;
    pid_t   runner_pid;
    int     user_id;
    int     cmd_id;
    char    cmd[MAX_CMD_LEN];
} Message;
 
typedef enum {
    RESP_SUBMITTED = 1,
    RESP_RUN       = 2,
    RESP_STATUS    = 3,
    RESP_SHUTSUBM  = 4,
    RESP_SHUTDOWN  = 5,
    RESP_DENIED    = 6
} RespType;
 
typedef struct {
    RespType type;
    int      cmd_id;
    char     body[MAX_CMD_LEN];
} Response;

int  createFifo(const char *path);
int  openFifo(const char *path, int mode);

#endif  //UTILS_H