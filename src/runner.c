#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h> 
#include <errno.h>

#include "utils.h"
#include "parser.h"
#include "executor.h"

/* escreve uma string no fd */
static void writestr(int fd, const char *s) {
    write(fd, s, strlen(s));
}

/* converte int para string e escreve no fd */
static void writeint(int fd, int val) {
    char buf[32];
    int i = 0;
    if (val == 0) { write(fd, "0", 1); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    for (int a = 0, b = i-1; a < b; a++, b--) {
        char c = buf[a]; buf[a] = buf[b]; buf[b] = c;
    }
    write(fd, buf, i);
}

/* ------------------------------------------------------------------ */
/*  Modo -e : executar um comando                                      */
/* ------------------------------------------------------------------ */
static int mode_exec(int argc, char *argv[]) {
    if (argc != 4) {
        writestr(2, "[runner] uso: ./runner -e <user-id> \"<command> [args...]\"\n");
        return 1;
    }

    pid_t my_pid  = getpid();
    int   user_id = atoi(argv[2]);
    int   cmd_id  = (int)my_pid;

    /* 1. cria FIFO privado */
    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);
    if (!createFifo(priv_fifo)) {
        writestr(2, "[runner] erro ao criar FIFO privado\n");
        return 1;
    }

    /* 2. constroi mensagem MSG_EXEC */
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_EXEC;
    msg.runner_pid = my_pid;
    msg.user_id    = user_id;
    msg.cmd_id     = cmd_id;
    strncpy(msg.cmd, argv[3], MAX_CMD_LEN - 1);

    /* 3. notifica utilizador: submetido */
    writestr(1, "[runner] command ");
    writeint(1, cmd_id);
    writestr(1, " submitted\n");

    /* 4. envia pedido ao controller */
    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        writestr(2, "[runner] nao foi possivel conectar ao controller\n");
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    /* 5. aguarda resposta do controller */
    int fd_priv = open(priv_fifo, O_RDONLY);
    if (fd_priv < 0) {
        writestr(2, "[runner] erro ao abrir FIFO privado\n");
        unlink(priv_fifo);
        return 1;
    }

    Response resp;
    /* primeira leitura — pode ser RESP_SUBMITTED ou RESP_RUN direto */
    read(fd_priv, &resp, sizeof(Response));

    if (resp.type == RESP_SUBMITTED) {
        /* ficou na fila. fecha e reabre o FIFO para bloquear até o controller abrir do outro lado quando houver slot disponível */
        close(fd_priv);
        fd_priv = open(priv_fifo, O_RDONLY);
        read(fd_priv, &resp, sizeof(Response));
    }

    if (resp.type != RESP_RUN) {
        writestr(2, "[runner] autorizacao recusada pelo controller\n");
        close(fd_priv);
        unlink(priv_fifo);
        return 1;
    }

    // Fechamos o FIFO privado após receber a autorização
    close(fd_priv);

    /* 6. notifica utilizador: em execução */
    writestr(1, "[runner] executing command ");
    writeint(1, cmd_id);
    writestr(1, "...\n");

    /* 7. tokeniza o comando e executa a pipeline completa */
    int tok_argc = 0;
    char **tokens = tokenize(argv[3], &tok_argc);
    if (tok_argc > 0) {
        Pipeline *p = parse_pipeline(tokens, tok_argc);
        execute_pipeline(p);
        free_pipeline(p);
    }

    /* 8. envia MSG_DONE ao controller */
    Message done;
    memset(&done, 0, sizeof(done));
    done.type       = MSG_DONE;
    done.runner_pid = my_pid;
    done.user_id    = user_id;
    done.cmd_id     = cmd_id;

    fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl >= 0) {
        write(fd_ctrl, &done, sizeof(done));
        close(fd_ctrl);
    }

    /* 9. notifica utilizador: terminado */
    writestr(1, "[runner] command ");
    writeint(1, cmd_id);
    writestr(1, " finished\n");

    unlink(priv_fifo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Modo -c : consultar comandos                                       */
/* ------------------------------------------------------------------ */
static int mode_status(void) {
    pid_t my_pid = getpid();
    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);
    
    if (!createFifo(priv_fifo)) {
        writestr(2, "[runner] erro ao criar FIFO privado\n");
        return 1;
    }

    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_STATUS;
    msg.runner_pid = my_pid;

    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        writestr(2, "[runner] nao foi possivel conectar ao controller\n");
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    int fd_priv = open(priv_fifo, O_RDONLY);
    if (fd_priv < 0) {
        writestr(2, "[runner] erro ao abrir FIFO privado\n");
        unlink(priv_fifo);
        return 1;
    }

    Response resp;
    memset(&resp, 0, sizeof(resp));
    read(fd_priv, &resp, sizeof(Response));
    writestr(1, resp.body);

    close(fd_priv);
    unlink(priv_fifo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Modo -s : pedir shutdown ao controller                            */
/* ------------------------------------------------------------------ */
static int mode_shutdown(void) {
    pid_t my_pid = getpid();
    char priv_fifo[RUNNER_FIFO_LEN];
    snprintf(priv_fifo, RUNNER_FIFO_LEN, RUNNER_FIFO_FMT, my_pid);
    
    if (!createFifo(priv_fifo)) {
        writestr(2, "[runner] erro ao criar FIFO privado\n");
        return 1;
    }

    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_SHUTDOWN;
    msg.runner_pid = my_pid;

    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        writestr(2, "[runner] nao foi possivel conectar ao controller\n");
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    writestr(1, "[runner] sent shutdown notification\n");
    writestr(1, "[runner] waiting for controller to shutdown...\n");

    int fd_priv = open(priv_fifo, O_RDONLY);
    if (fd_priv >= 0) {
        Response resp;
        memset(&resp, 0, sizeof(resp));
        read(fd_priv, &resp, sizeof(Response));
        close(fd_priv);
    }

    writestr(1, "[runner] controller exited.\n");
    unlink(priv_fifo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        writestr(2, "uso: ./runner -e <user-id> \"<command> [args...]\"\n");
        writestr(2, "     ./runner -c\n");
        writestr(2, "     ./runner -s\n");
        return 1;
    }

    if (strcmp(argv[1], "-e") == 0)      return mode_exec(argc, argv);
    else if (strcmp(argv[1], "-c") == 0) return mode_status();
    else if (strcmp(argv[1], "-s") == 0) return mode_shutdown();
    else {
        writestr(2, "[runner] opcao invalida\n");
        return 1;
    }
}