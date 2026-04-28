#ifndef PARSER_H
#define PARSER_H

#include "utils.h"

typedef struct {
    char **args;
    char *input;
    char *output;
    char *error;
} Command;

typedef struct {
    Command *commands;
    int num_commands;
} Pipeline;

Pipeline *parse_pipeline(char **argv, int argc);
void free_pipeline(Pipeline *p);
void print_pipeline(Pipeline *p);
char **tokenize(const char *str, int *argc_out);

#endif