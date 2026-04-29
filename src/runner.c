#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "-e") != 0) {
        write(2, "Usage: ./runner -e <user-id> \"<command>\"\n", 41);
        return 1;
    }

    int user_id = atoi(argv[2]);
    const char *cmd = argv[3];

    int fd = openFifo(CONTROLLER_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("[runner] open controller fifo");
        return 1;
    }

    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type       = MSG_EXEC;
    msg.runner_pid = getpid();
    msg.user_id    = user_id;
    strncpy(msg.cmd, cmd, 256);

    write(1, "[runner] sending message...\n", 28);

    if (write(fd, &msg, sizeof(Message)) != (ssize_t)sizeof(Message)) {
        perror("[runner] write");
        close(fd);
        return 1;
    }

    write(1, "[runner] message sent\n", 22);
    close(fd);
    return 0;
}