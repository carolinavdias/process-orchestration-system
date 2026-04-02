#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char* argv[]){

    if(argc != 3){
        write(2, "Usage: ./controller <parallel-commands> <sched-policy>\n", 56);
        return 1;
    }

    return 0;
}