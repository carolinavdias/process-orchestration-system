#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "utils.h"

int createFifo(const char* path){
    unlink(path);

    return mkfifo(path, 0666);
}

int openFifo(const char* path, int mode){
    return open(path, mode);
}
