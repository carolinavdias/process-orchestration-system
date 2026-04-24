#include "parser.h"
#include "executor.h"
#include <unistd.h>

int main() {
    /* teste 1: comando simples */
    write(1, "--- teste 1: echo simples ---\n", 30);
    char *t1[] = {"echo", "ola mundo"};
    Pipeline *p1 = parse_pipeline(t1, 2);
    execute_pipeline(p1);
    free_pipeline(p1);

    /* teste 2: output para ficheiro */
    write(1, "--- teste 2: echo > ficheiro ---\n", 33);
    char *t2[] = {"echo", "hello", ">", "/tmp/test_out.txt"};
    Pipeline *p2 = parse_pipeline(t2, 4);
    execute_pipeline(p2);
    free_pipeline(p2);
    /* verifica se o ficheiro foi criado */
    char *t2b[] = {"cat", "/tmp/test_out.txt"};
    Pipeline *p2b = parse_pipeline(t2b, 2);
    execute_pipeline(p2b);
    free_pipeline(p2b);

    /* teste 3: pipe entre dois comandos */
    write(1, "--- teste 3: grep | wc -l ---\n", 30);
    char *t3[] = {"grep", "root", "/etc/passwd", "|", "wc", "-l"};
    Pipeline *p3 = parse_pipeline(t3, 6);
    execute_pipeline(p3);
    free_pipeline(p3);

    /* teste 4: input de ficheiro */
    write(1, "--- teste 4: wc -l < ficheiro ---\n", 34);
    char *t4[] = {"wc", "-l", "<", "/etc/passwd"};
    Pipeline *p4 = parse_pipeline(t4, 4);
    execute_pipeline(p4);
    free_pipeline(p4);

    return 0;
}