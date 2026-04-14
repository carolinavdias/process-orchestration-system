#ifndef UTILS_H
#define UTILS_H

#define CONTROLLER_FIFO "/tmp/controller_fifo"

int createFifo(const char* path);

int openFifo(const char* path, const int mode);

#define MAX_ARGS         32
#define MAX_ARG_LEN      64

typedef enum {
    MSG_EXEC     = 1,   /* runner quer executar um comando  (-e) */
    MSG_DONE     = 2,   /* runner terminou a execução            */
    MSG_STATUS   = 3,   /* runner quer listar comandos       (-c) */
    MSG_SHUTDOWN = 4    /* runner pede terminação            (-s) */
} MsgType;

typedef struct {
    MsgType  type;
    pid_t    runner_pid;          /* PID do runner — para o FIFO privado   */
    int      user_id;
    int      cmd_id;
    char     cmd[256];            /* nome do executável                    */
    char     args[MAX_ARGS][MAX_ARG_LEN];
    int      argc;
} Message;

#endif  //UTILS_H