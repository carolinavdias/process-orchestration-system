#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "scheduler.h"

static void notify_waiting_runners(void) {
    int cmd_id;
    pid_t pid;
    while ((pid = scheduler_pop_waiting(&cmd_id)) != -1) {
        char fifo[RUNNER_FIFO_LEN];
        snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, pid);
        int rfd = open(fifo, O_WRONLY);
        if (rfd != -1) {
            Response resp = {0};
            resp.type   = RESP_SHUTDOWN;
            resp.cmd_id = cmd_id;
            if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                perror("[controller] write response");
            }
            close(rfd);
        }
    }
}

int controller(int fd){
    write(1, "[controller] waiting for runners...\n", 36);

    int shutdown_requested  = 0;
    pid_t shutdown_requester = -1;

    while(1){
        if (shutdown_requested && scheduler_empty()) {
            notify_waiting_runners();
            char fifo[RUNNER_FIFO_LEN];
            snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, shutdown_requester);
            int rfd = open(fifo, O_WRONLY);
            if (rfd != -1) {
                Response resp = {0};
                resp.type = RESP_SHUTDOWN;
                if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                    perror("[controller] write response");
                }
                close(rfd);
            }
            break;
        }

        Message msg;
        ssize_t r = read(fd, &msg, sizeof(Message));

        if (r == 0 || (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
            usleep(10000);
            continue;
        }
        else if (r == -1 || r != (ssize_t)sizeof(msg)) {
            perror("[controller] read");
            return -1;
        }
        else {
            switch (msg.type) {
                case MSG_EXEC: {
                    if (shutdown_requested) {
                        write(1, "[controller] denying request, shutdown in progress\n", 51);
                        char fifo[RUNNER_FIFO_LEN];
                        snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, msg.runner_pid);
                        int rfd = open(fifo, O_WRONLY);
                        if (rfd != -1) {
                            Response resp = {0};
                            resp.type = RESP_DENIED;
                            if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                                perror("[controller] write response");
                            }
                            close(rfd);
                        }
                        break;
                    }

                    char buf[128];
                    int n = snprintf(buf, sizeof(buf),
                        "[controller] exec request from user=%d cmd='%s'\n",
                        msg.user_id, msg.cmd);
                    write(1, buf, n);

                    int cmd_id = scheduler_enqueue(&msg);
                    if (cmd_id == -1) {
                        write(2, "[controller] queue error\n", 26);
                        break;
                    }

                    /* notifica o runner que o comando foi submetido */
                    char fifoS[RUNNER_FIFO_LEN];
                    snprintf(fifoS, sizeof(fifoS), RUNNER_FIFO_FMT, msg.runner_pid);
                    int o = open(fifoS, O_WRONLY);
                    if (o != -1) {
                        Response resp = {0};
                        resp.type   = RESP_SUBMITTED;
                        resp.cmd_id = cmd_id;
                        if (write(o, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                            perror("[controller] write response");
                        }
                        close(o);
                    }

                    /* tenta despachar, pode haver slot livre */
                    CmdNode *entry = scheduler_next();
                    if (entry != NULL) {
                        char fifo[RUNNER_FIFO_LEN];
                        snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, entry->msg.runner_pid);
                        int rfd = open(fifo, O_WRONLY);
                        if (rfd != -1) {
                            Response resp = {0};
                            resp.type   = RESP_RUN;
                            resp.cmd_id = entry->msg.cmd_id;
                            if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                                perror("[controller] write response");
                            }
                            close(rfd);
                        }
                    }
                    break;
                }

                case MSG_DONE: {
                    long ms = scheduler_done(msg.cmd_id);

                    char buf[128];
                    int n = snprintf(buf, sizeof(buf),
                        "[controller] done cmd=%d duration=%ldms\n", msg.cmd_id, ms);
                    write(1, buf, n);

                    /* registo persistente */
                    int lfd = open("logs/commands.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (lfd != -1) {
                        char log[128];
                        int ln = snprintf(log, sizeof(log),
                            "user=%d cmd=%d duration=%ldms\n",
                            msg.user_id, msg.cmd_id, ms);
                        if (write(lfd, log, ln) != ln) {
                            perror("[controller] write log");
                        }
                        close(lfd);
                    }

                    if (shutdown_requested){
                        break;
                    }

                    /* slot livre,tenta despachar próximo */
                    CmdNode *entry = scheduler_next();
                    if (entry != NULL) {
                        char fifo[RUNNER_FIFO_LEN];
                        snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, entry->msg.runner_pid);
                        int rfd = open(fifo, O_WRONLY);
                        if (rfd != -1) {
                            Response resp = {0};
                            resp.type   = RESP_RUN;
                            resp.cmd_id = entry->msg.cmd_id;
                            if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                                perror("[controller] write response");
                            }
                            close(rfd);
                        }
                    }
                    break;
                }

                case MSG_STATUS: {
                    write(1, "[controller] status request\n", 28);
                    char fifo[RUNNER_FIFO_LEN];
                    snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, msg.runner_pid);
                    int rfd = open(fifo, O_WRONLY);
                    if (rfd != -1) {
                        Response resp = {0};
                        resp.type = RESP_STATUS;
                        scheduler_status(resp.body, sizeof(resp.body));
                        if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                            perror("[controller] write response");
                        }
                        close(rfd);
                    }
                    break;
                }

                case MSG_SHUTDOWN: {
                    if (shutdown_requested) {
                        write(1, "[controller] denying request, shutdown in progress\n", 51);
                        char fifo[RUNNER_FIFO_LEN];
                        snprintf(fifo, sizeof(fifo), RUNNER_FIFO_FMT, msg.runner_pid);
                        int rfd = open(fifo, O_WRONLY);
                        if (rfd != -1) {
                            Response resp = {0};
                            resp.type = RESP_DENIED;
                            if (write(rfd, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                                perror("[controller] write response");
                            }
                            close(rfd);
                        }
                        break;
                    }
                    write(1, "[controller] shutdown requested\n", 32);
                    char fifoS[RUNNER_FIFO_LEN];
                    snprintf(fifoS, sizeof(fifoS), RUNNER_FIFO_FMT, msg.runner_pid);
                    int o = open(fifoS, O_WRONLY);
                    if (o != -1) {
                        Response resp = {0};
                        resp.type = RESP_SHUTSUBM;
                        if (write(o, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
                            perror("[controller] write response");
                        }
                        close(o);
                    }
                    shutdown_requested = 1;
                    shutdown_requester = msg.runner_pid;
                    break;
                }

                default:
                    write(2, "[controller] unknown message type\n", 34);
                    break;
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[]){
    if (argc != 3) {
        write(2, "Usage: ./controller <parallel-commands> <sched-policy>\n", 55);
        return 1;
    }

    int max_parallel = atoi(argv[1]);
    if (max_parallel <= 0) {
        write(2, "[controller] error: parallel-commands must be > 0\n", 50);
        return 1;
    }

    const char *sched_policy = argv[2];
    if (strcmp(sched_policy, "fcfs") != 0 && strcmp(sched_policy, "rr") != 0) {
        write(2, "[controller] error: sched-policies available: fcfs | rr\n", 57);
        return 1;
    }

    if (createFifo(CONTROLLER_FIFO) == -1 && errno != EEXIST) {
        perror("[controller] mkfifo");
        return 1;
    }

    int fd = openFifo(CONTROLLER_FIFO, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        perror("[controller] open fifo");
        return 1;
    }

    char buf[128];
    int n = snprintf(buf, sizeof(buf),
        "[controller] started | policy = %s  max_parallel = %d\n",
        sched_policy, max_parallel);
    write(1, buf, n);

    scheduler_init(max_parallel, sched_policy);

    int cont = controller(fd);

    close(fd);
    unlink(CONTROLLER_FIFO);
    scheduler_destroy();

    write(1, "[controller] exited\n", 20);
    return (cont == 0) ? 0 : 1;
}
