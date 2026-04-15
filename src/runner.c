#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include "utils.h"

// IMPLEMENTAR QUE DETETE OS SIMBOLOS >, 2>, < OU |
// Garantir que o Runner imprime exatamente as etiquetas [runner] seguidas do estado: submitted, executing e finished
// scripts

// usa o PID como cmd_id (ja esta feito)


/* constroi caminho do FIFO privado do runner */
static void private_fifo_path(char *buf, size_t size, pid_t pid) {

    /* ex: /tmp/runner_12345 */
    int i = 0;
    const char *prefix = "/tmp/runner_";

    while (*prefix) buf[i++] = *prefix++;

    /* converte pid para string -> sem sprintf */
    char tmp[32];
    int n = 0;
    pid_t p = pid;

    if (p == 0)  tmp[n++] = '0'; 

    while (p > 0) { 
        tmp[n++] = '0' + (p % 10); 
        p /= 10;
    }

    /* inverte */
    for (int a = 0, b = n-1; a < b; a++, b--) {
        char c = tmp[a]; tmp[a] = tmp[b]; tmp[b] = c;
    }

    for (int j = 0; j < n && i < (int)size - 1; j++) buf[i++] = tmp[j];
    buf[i] = '\0';

}

/* escreve uma string no fd */
static void writestr(int fd, const char *s) {
    write(fd, s, strlen(s));
}

/* converte int para string e escreve no fd */
static void writeint(int fd, int val) {
    
    char buf[32];
    int i = 0;

    if (val == 0) { 
        write(fd, "0", 1); 
        return; 
    }

    while (val > 0) { 
        buf[i++] = '0' + (val % 10); 
        val /= 10;
    }

    /* inverte e escreve */
    for (int a = 0, b = i-1; a < b; a++, b--) {
        char c = buf[a]; buf[a] = buf[b]; buf[b] = c;
    }

    write(fd, buf, i);
}

// ---------------------------------------------------------------------------

/* ------------------------------------------------------------------ */
/*  Modo -e : executar um comando                                      */
/* ------------------------------------------------------------------ */
static int mode_exec(int argc, char *argv[]) {
    /*
     * argv layout esperado:
     *   argv[0] = "./runner"
     *   argv[1] = "-e"
     *   argv[2] = "<user-id>"
     *   argv[3] = "<command>"          (pode ser string com pipe/redirect)
     *   argv[4..] = argumentos extras  (quando não vêm em string única)
     */
    if (argc < 4) {
        writestr(2, "[runner] uso: ./runner -e <user-id> <command> [args...]\n");
        return 1;
    }

    pid_t my_pid = getpid();
    int   user_id = atoi(argv[2]);
    int   cmd_id  = (int)my_pid;

    /* --- 1. Criar FIFO privado --- */
    char priv_fifo[128];
    private_fifo_path(priv_fifo, sizeof(priv_fifo), my_pid);
    if (!createFifo(priv_fifo)) {
        writestr(2, "[runner] erro ao criar FIFO privado\n");
        return 1;
    }

    /* --- 2. Construir mensagem MSG_EXEC --- */
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_EXEC;
    msg.runner_pid = my_pid;
    msg.user_id    = user_id;
    msg.cmd_id     = cmd_id;

    strncpy(msg.cmd, argv[3], sizeof(msg.cmd) - 1);

    msg.argc = argc - 3;
    for (int i = 0; i < msg.argc && i < MAX_ARGS; i++) {
        strncpy(msg.args[i], argv[3 + i], MAX_ARG_LEN - 1);
    }

    /* --- 3. Notificar utilizador: submetido (ANTES de enviar ao controller) --- */
    writestr(1, "[runner] command ");
    writeint(1, cmd_id);
    writestr(1, " submitted\n");

    /* --- 4. Enviar pedido ao controller --- */
    int fd_ctrl = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd_ctrl < 0) {
        writestr(2, "[runner] nao foi possivel conectar ao controller\n");
        unlink(priv_fifo);
        return 1;
    }
    write(fd_ctrl, &msg, sizeof(msg));
    close(fd_ctrl);

    /* --- 5. Aguardar autorização (bloqueia aqui) --- */
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

    /* --- 6. Notificar utilizador: em execução --- */
    writestr(1, "[runner] executing command ");
    writeint(1, cmd_id);
    writestr(1, "...\n");

    /* --- 7. fork + execvp --- */
    pid_t child = fork();
    if (child < 0) {
        writestr(2, "[runner] erro no fork\n");
        unlink(priv_fifo);
        return 1;
    }

    if (child == 0) {
        char *exec_args[MAX_ARGS + 1];
        for (int i = 0; i < msg.argc; i++) exec_args[i] = msg.args[i];
        exec_args[msg.argc] = NULL;

        execvp(msg.cmd, exec_args);
        writestr(2, "[runner] erro ao executar comando\n");
        exit(1);
    }

    waitpid(child, NULL, 0);

    /* --- 8. Notificar controller: terminado --- */
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

    /* --- 9. Notificar utilizador: terminado --- */
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

    /* Enviar pedido de status */
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

    /* Receber e imprimir resposta */
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

    /* Aguardar confirmação do controller no FIFO privado */
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
        writestr(2, "uso: ./runner -e <user-id> <command> [args...]\n");
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