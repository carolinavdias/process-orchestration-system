#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char *argv[]){

    if(argc < 2 || strlen(argv[1]) != 2){
        write(2, "Usage: ./runner -e <user-id> <command> [args...]\n", 50);
        write(2, "Usage: ./runner -c\n", 20);
        write(2, "Usage: ./runner -s\n", 20);
        return 1;
    }

    if (strcmp(argv[1], "-e") == 0 && argc >= 4) {

    }
    else if (strcmp(argv[1], "-c") == 0 && argc == 2) {
        
    }
    else if (strcmp(argv[1], "-s") == 0 && argc == 2) {
        
    }
    else {
        write(2, "Invalid option\n", 16);
        write(2, "Usage: ./runner -e <user-id> <command> [args...]\n", 50);
        write(2, "Usage: ./runner -c\n", 20);
        write(2, "Usage: ./runner -s\n", 20);
        return 1;
    }

    return 0;
}