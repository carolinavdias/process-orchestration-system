#include "parser.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    //teste 1: comando simples
    char *t1[] = {"echo", "ola"};
    Pipeline *p1 = parse_pipeline(t1, 2);
    write(1, "--- teste 1: comando simples ---\n", 33);
    print_pipeline(p1);
    free_pipeline(p1);

    //teste 2: redirecionamento de output
    char *t2[] = {"echo", "ola", ">", "out.txt"};
    Pipeline *p2 = parse_pipeline(t2, 4);
    write(1, "--- teste 2: redireccionamento > ---\n", 37);
    print_pipeline(p2);
    free_pipeline(p2);

    //teste 3: pipe entre dois comandos
    char *t3[] = {"grep", "system", "/etc/passwd", "|", "wc", "-l"};
    Pipeline *p3 = parse_pipeline(t3, 6);
    write(1, "--- teste 3: pipe | ---\n", 24);
    print_pipeline(p3);
    free_pipeline(p3);

    //teste 4: tudo junto :)
    char *t4[] = {"grep", "system", "<", "in.txt", "|", "wc", "-l", ">", "out.txt"};
    Pipeline *p4 = parse_pipeline(t4, 9);
    write(1, "--- teste 4: < | > juntos ---\n", 30);
    print_pipeline(p4);
    free_pipeline(p4);

    return 0;
}