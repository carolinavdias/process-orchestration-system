#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include "utils.h"
#include "parser.h"
#include "executor.h"

/* constroi caminho do FIFO privado do runner */
static void private_fifo_path(char *buf, size_t size, pid_t pid) {
    int i = 0;
    const char *prefix = "/tmp/runner_";
    while (*prefix) buf[i++] = *prefix++;

    char tmp[32];
    int n = 0;
    pid_t p = pid;

    if (p == 0) tmp[n++] = '0';
    while (p > 0) { tmp[n++] = '0' + (p % 10); p /= 10; }

    for (int a = 0, b = n-1; a < b; a++, b--) {
        char c = tmp[a]; tmp[a] = tmp[b]; tmp[b] = c;
    }
    for (int j = 0; j < n && i < (int)size - 1; j++) buf[i++] = tmp[j];
    buf[i] = '\0';
}

static void writestr(int fd, const char *s) {
    write(fd, s, strlen(s));
}

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
    /*
     * argv layout (novo enunciado):
     *   argv[0] = "./runner"
     *   argv[1] = "-e"
     *   argv[2] = "<user-id>"
     *   argv[3] = "<command> <arg1> ..." (string única entre aspas)
     */
    if (argc != 4) {
        writestr(2, "[runner] uso: ./runner -e <user-id> \"<command> [args...]\"\n");
        return 1;
    }

    pid_t my_pid = getpid();
    int   user_id = atoi(argv[2]);
    int   cmd_id  = (int)my_pid;

    /* 1. cria FIFO privado */
    char priv_fifo[128];
    private_fifo_path(priv_fifo, sizeof(priv_fifo), my_pid);
    if (!createFifo(priv_fifo)) {
        writestr(2, "[runner] erro ao criar FIFO privado\n");
        return 1;
    }

    /* 2. tokeniza a string do comando */
    int tok_argc = 0;
    char **tokens = tokenize(argv[3], &tok_argc);
    if (tok_argc == 0) {
        writestr(2, "[runner] comando vazio\n");
        unlink(priv_fifo);
        return 1;
    }

    /* 3. constroi mensagem para o controller */
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_EXEC;
    msg.runner_pid = my_pid;
    msg.user_id    = user_id;
    msg.cmd_id     = cmd_id;

    /* guarda o comando completo (string original) */
    strncpy(msg.cmd, argv[3], sizeof(msg.cmd) - 1);
    msg.argc = tok_argc;
    for (int i = 0; i < tok_argc && i < MAX_ARGS; i++) {
        strncpy(msg.args[i], tokens[i], MAX_ARG_LEN - 1);
    }

    /* 4. notifica utilizador: submetido */
    writestr(1, "[runner] command ");
    writeint(1, cmd_id);
    writestr(1, " submitted\n");

    /* 5. envia pedido ao controller */
    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        writestr(2, "[runner] nao foi possivel conectar ao controller\n");
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    /* 6. aguarda autorização */
    int fd_priv = open(priv_fifo, O_RDONLY);
    if (fd_priv < 0) {
        writestr(2, "[runner] erro ao abrir FIFO privado\n");
        unlink(priv_fifo);
        return 1;
    }
    int auth = 0;
    read(fd_priv, &auth, sizeof(int));
    close(fd_priv);

    if (auth != 1) {
        writestr(2, "[runner] autorizacao recusada pelo controller\n");
        unlink(priv_fifo);
        return 1;
    }

    /* 7. notifica utilizador: em execução */
    writestr(1, "[runner] executing command ");
    writeint(1, cmd_id);
    writestr(1, "...\n");

    /* 8. parse + execute pipeline (suporte a |, >, 2>, <) */
    Pipeline *p = parse_pipeline(tokens, tok_argc);
    execute_pipeline(p);
    free_pipeline(p);

    /* 9. envia MSG_DONE ao controller */
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

    /* 10. notifica utilizador: terminado */
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

    char priv_fifo[128];
    private_fifo_path(priv_fifo, sizeof(priv_fifo), my_pid);
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

    char buf[4096];
    ssize_t n;
    while ((n = read(fd_priv, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }
    close(fd_priv);
    unlink(priv_fifo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Modo -s : pedir shutdown ao controller                            */
/* ------------------------------------------------------------------ */
static int mode_shutdown(void) {
    pid_t my_pid = getpid();

    char priv_fifo[128];
    private_fifo_path(priv_fifo, sizeof(priv_fifo), my_pid);
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
        int ack = 0;
        read(fd_priv, &ack, sizeof(int));
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