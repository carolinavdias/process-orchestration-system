#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char **args;    //comando e argumentos (null-terminated)
    char *input;    //ficheiro para < (ou NULL)
    char *output;   //ficheiro para > (ou NULL)
    char *error;    //ficheiro para 2> (ou NULL)
} Command;

typedef struct {
    Command *commands;  //array de comandos na pipeline
    int num_commands;   //número de comandos
} Pipeline;

Pipeline *parse_pipeline(char **argv, int argc);
void free_pipeline(Pipeline *p);
void print_pipeline(Pipeline *p);   //para debug

#endif