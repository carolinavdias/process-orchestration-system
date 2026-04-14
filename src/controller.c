#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "utils.h"

int main(int argc, char* argv[]){

    if(argc != 3){
        perror("Usage: ./controller <parallel-commands> <sched-policy>\n");
        return 1;
    }

    if (!createFifo(CONTROLLER_FIFO)) {
        perror("> Error creating fifo <\n");
        return 1;
    }

    int fd = openFifo(CONTROLLER_FIFO, O_RDWR);

    if (!fd){
        perror("> Error opening fifo <\n");
        return 1;
    }

    write(1, "[Controller] started\n", 22);
    write(1, "\n", 1);
    char output[256];
    snprintf(output, sizeof(output), "[Controller] sched-policy: %s\n", argv[2]);
    write(1, output, strlen(output));
    snprintf(output, sizeof(output), "[Controller] max parallel comands: %d\n", atoi(argv[1]));
    write(1, output, strlen(output));
    write(1, "\n", 1);

    Message msg;
    while(1){
        ssize_t r = read(fd, &msg, sizeof(Message));

        if (r == 0)  continue;                     /* EOF espúrio, ignora    */
        else if (r == -1) { 
            perror("> Error reading message <"); 
            break; 
        }
        else{
            write(1, "[Controller] received message\n", 31);

            
        }
    }

    close(fd);
    unlink(CONTROLLER_FIFO);

    return 0;
}