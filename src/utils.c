#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "utils.h"

int createFifo(const char* path){
    int fd = mkfifo(path, 0644);

    if (fd < 0) {
      return 0;
    }

    return 1;
}

int openFifo(const char* path, const int mode){
    int fd = open(path, mode);

    if(fd < 0){
        return 0;
    }

    return fd;
}