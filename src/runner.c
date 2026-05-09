#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "executor/parser.h"
#include "executor/executor.h"

int mode_exec(int argc, char *argv[]) {
    (void)argc;   
    pid_t my_pid  = getpid();
    int   user_id = atoi(argv[2]);

    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);
    if (createFifo(priv_fifo) == -1 && errno != EEXIST) {
        write(2, "[runner] error creating private FIFO\n", 37);
        return 1;
    }
    
    // abre fifo privado para receber resposta do controller
    int fd_priv = open(priv_fifo, O_RDWR);
    if (fd_priv < 0) {
        write(2, "[runner] error opening private FIFO\n", 36);
        unlink(priv_fifo);
        return 1;
    }

    Message msg = {0};
    msg.type       = MSG_EXEC;
    msg.runner_pid = my_pid;
    msg.user_id    = user_id;
    strncpy(msg.cmd, argv[3], MAX_CMD_LEN - 1);

    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        write(2, "[runner] error connecting to controller\n", 40);
        close(fd_priv);
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    Response resp = {0};
    read(fd_priv, &resp, sizeof(resp));

    if (resp.type == RESP_DENIED) {
        write(2, "[runner] command denied: controller is shutting down\n", 53);
        close(fd_priv);
        unlink(priv_fifo);
        return 1;
    }
    else if (resp.type == RESP_SHUTDOWN) {
        write(2, "[runner] command cancelled: controller shut down\n", 49);
        close(fd_priv);
        unlink(priv_fifo);
        return 1;
    }

    // protocolo de dois passos: submitted->run
    if (resp.type == RESP_SUBMITTED) {
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "[runner] command %d submitted\n", resp.cmd_id);
        write(1, buf, n);

        read(fd_priv, &resp, sizeof(resp));
        if (resp.type != RESP_RUN) {
            write(2, "[runner] unexpected response from controller\n", 45);
            close(fd_priv);
            unlink(priv_fifo);
            return 1;
        }
    }

    close(fd_priv);

    char buf[64];
    int n = snprintf(buf, sizeof(buf), "[runner] executing command %d...\n", resp.cmd_id);
    write(1, buf, n);

    // executa e notifica conclusão
    int tok_argc = 0;
    char **tokens = tokenize(argv[3], &tok_argc);
    if (tok_argc > 0) {
        Pipeline *p = parse_pipeline(tokens, tok_argc);
        execute_pipeline(p);
        free_pipeline(p);
    }
    free(tokens);

    Message done = {0};
    done.type       = MSG_DONE;
    done.runner_pid = my_pid;
    done.user_id    = user_id;
    done.cmd_id     = resp.cmd_id;

    fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl >= 0) {
        write(fd_ctrl, &done, sizeof(done));
        close(fd_ctrl);
    }

    n = snprintf(buf, sizeof(buf), "[runner] command %d finished\n", resp.cmd_id);
    write(1, buf, n);

    unlink(priv_fifo);
    return 0;
}

int mode_status(void) {
    pid_t my_pid = getpid();
    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);

    if (createFifo(priv_fifo) == -1 && errno != EEXIST) {
        write(2, "[runner] error creating private FIFO\n", 37);
        return 1;
    }

    Message msg = {0};
    msg.type       = MSG_STATUS;
    msg.runner_pid = my_pid;

    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        write(2, "[runner] error connecting to controller\n", 40);
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    int fd_priv = open(priv_fifo, O_RDONLY);
    if (fd_priv < 0) {
        write(2, "[runner] error opening private FIFO\n", 36);
        unlink(priv_fifo);
        return 1;
    }

    Response resp = {0};
    read(fd_priv, &resp, sizeof(Response));
    write(1, resp.body, strlen(resp.body));

    close(fd_priv);
    unlink(priv_fifo);
    return 0;
}

int mode_shutdown(void) {
    pid_t my_pid = getpid();
    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);

    if (createFifo(priv_fifo) == -1 && errno != EEXIST) {
        write(2, "[runner] error creating private FIFO\n", 37);
        return 1;
    }

    Message msg = {0};
    msg.type       = MSG_SHUTDOWN;
    msg.runner_pid = my_pid;

    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        write(2, "[runner] error connecting to controller\n", 40);
        unlink(priv_fifo);
        return 1;
    }
    int w = write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);
    if (w <= 0) {
        write(2, "[runner] error writing to controller\n", 37);
        unlink(priv_fifo);
        return 1;
    }

    write(1, "[runner] sent shutdown notification\n", 36);

    int fd_priv = open(priv_fifo, O_RDWR);
    if (fd_priv < 0) {
        write(2, "[runner] error opening private FIFO\n", 36);
        unlink(priv_fifo);
        return 1;
    }

    Response resp = {0};
    read(fd_priv, &resp, sizeof(resp));

    if (resp.type == RESP_DENIED) {
        write(2, "[runner] shutdown refused: controller already shutting down\n", 60);
        close(fd_priv);
        unlink(priv_fifo);
        return 1;
    }
    else if (resp.type == RESP_SHUTSUBM) {
        write(1, "[runner] waiting for controller to shutdown...\n", 47);
        read(fd_priv, &resp, sizeof(resp));
        if (resp.type != RESP_SHUTDOWN) {
            write(2, "[runner] unexpected response from controller\n", 45);
            close(fd_priv);
            unlink(priv_fifo);
            return 1;
        }
    }

    close(fd_priv);

    write(1, "[runner] controller exited.\n", 28);
    unlink(priv_fifo);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 && argc > 4) {
        write(2, "Usage: ./runner -e <user-id> \"<command> [args...]\"\n", 52);
        write(2, "       ./runner -c\n", 20);
        write(2, "       ./runner -s\n", 20);
        return 1;
    }

    if (strcmp(argv[1], "-e") == 0)      return mode_exec(argc, argv);
    else if (strcmp(argv[1], "-c") == 0) return mode_status();
    else if (strcmp(argv[1], "-s") == 0) return mode_shutdown();
    else {
        write(2, "[runner] invalid option\n", 24);
        return 1;
    }
}
