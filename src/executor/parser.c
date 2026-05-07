#include "executor/parser.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char **tokenize(const char *str, int *argc_out) {
    size_t len = strlen(str) + 1;
    char **tokens = malloc((MAX_ARGS + 1) * sizeof(char *) + len);
    if (tokens == NULL) {
        *argc_out = 0;
        return NULL;
    }
    char *buf = (char *)(tokens + MAX_ARGS + 1);

    strcpy(buf, str);

    int n = 0;

    char *tok = strtok(buf, " \t");
    while (tok != NULL && n < MAX_ARGS) {
        tokens[n++] = tok;
        tok = strtok(NULL, " \t");
    }
    tokens[n] = NULL;
    *argc_out = n;
    return tokens;
}

Pipeline *parse_pipeline(char **argv, int argc) {
    Pipeline *p = malloc(sizeof(Pipeline));
    p->num_commands = 1;

    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], "|") == 0)
            p->num_commands++;

    p->commands = malloc(p->num_commands * sizeof(Command));

    for (int i = 0; i < p->num_commands; i++) {
        p->commands[i].args   = malloc((argc + 1) * sizeof(char *));
        p->commands[i].input  = NULL;
        p->commands[i].output = NULL;
        p->commands[i].error  = NULL;
    }

    int cmd_idx = 0;
    int arg_idx = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            p->commands[cmd_idx].args[arg_idx] = NULL;
            cmd_idx++;
            arg_idx = 0;
        } else if (strcmp(argv[i], ">") == 0 && i + 1 < argc) {
            p->commands[cmd_idx].output = argv[++i];
        } else if (strcmp(argv[i], "2>") == 0 && i + 1 < argc) {
            p->commands[cmd_idx].error = argv[++i];
        } else if (strcmp(argv[i], "<") == 0 && i + 1 < argc) {
            p->commands[cmd_idx].input = argv[++i];
        } else {
            p->commands[cmd_idx].args[arg_idx++] = argv[i];
        }
    }
    p->commands[cmd_idx].args[arg_idx] = NULL;
    return p;
}

void free_pipeline(Pipeline *p) {
    for (int i = 0; i < p->num_commands; i++)
        free(p->commands[i].args);
    free(p->commands);
    free(p);
}

void print_pipeline(Pipeline *p) {
    for (int i = 0; i < p->num_commands; i++) {
        write(1, "comando: ", 9);
        for (int j = 0; p->commands[i].args[j] != NULL; j++) {
            write(1, p->commands[i].args[j], strlen(p->commands[i].args[j]));
            write(1, " ", 1);
        }
        write(1, "\n", 1);
        if (p->commands[i].input)  { write(1, "  input: ",  9); write(1, p->commands[i].input,  strlen(p->commands[i].input));  write(1, "\n", 1); }
        if (p->commands[i].output) { write(1, "  output: ", 10); write(1, p->commands[i].output, strlen(p->commands[i].output)); write(1, "\n", 1); }
        if (p->commands[i].error)  { write(1, "  error: ",  9); write(1, p->commands[i].error,  strlen(p->commands[i].error));  write(1, "\n", 1); }
    }
}
