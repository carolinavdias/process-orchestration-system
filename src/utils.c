#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "utils.h"

int createFifo(const char* path){
    // 1. Tentar remover o FIFO se ele já existir de uma execução anterior
    unlink(path); 

    // 2. Criar o FIFO e verificar se o retorno é 0 (sucesso)
    if (mkfifo(path, 0666) == 0) {
        return 1; // Sucesso para o teu "if" no runner
    }
    
    return 0; // Falha
}

int openFifo(const char* path, int mode){
    return open(path, mode);
}