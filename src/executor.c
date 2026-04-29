#include "executor.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

int execute_pipeline(Pipeline *p) {
    int num = p->num_commands;

    /* array de pipes: precisamos de (num-1) pipes */
    int pipes[num - 1][2];
    for (int i = 0; i < num - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            write(2, "[executor] erro ao criar pipe\n", 30);
            return -1;
        }
    }

    pid_t pids[num];

    for (int i = 0; i < num; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            write(2, "[executor] erro no fork\n", 24);
            return -1;
        }

        if (pids[i] == 0) {
            /* processo filho */

            /* redireccionamento de input */
            if (p->commands[i].input != NULL) {
                int fd = open(p->commands[i].input, O_RDONLY);
                if (fd < 0) {
                    write(2, "[executor] erro ao abrir ficheiro de input\n", 43);
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            } else if (i > 0) {
                /* lê do pipe anterior */
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            /* redireccionamento de output */
            if (p->commands[i].output != NULL) {
                int fd = open(p->commands[i].output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    write(2, "[executor] erro ao abrir ficheiro de output\n", 44);
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else if (i < num - 1) {
                /* escreve para o pipe seguinte */
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            /* redireccionamento de stderr */
            if (p->commands[i].error != NULL) {
                int fd = open(p->commands[i].error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    write(2, "[executor] erro ao abrir ficheiro de erro\n", 42);
                    exit(1);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            /* fecha todos os pipes no filho */
            for (int j = 0; j < num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(p->commands[i].args[0], p->commands[i].args);
            write(2, "[executor] erro ao executar comando\n", 36);
            exit(1);
        }
    }

    /* processo pai - fecha todos os pipes */
    for (int i = 0; i < num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /* espera por todos os filhos */
    for (int i = 0; i < num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return 0;
}