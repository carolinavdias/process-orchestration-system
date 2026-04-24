#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/* executa uma pipeline completa com suporte a |, >, 2>, < */
int execute_pipeline(Pipeline *p);

#endif