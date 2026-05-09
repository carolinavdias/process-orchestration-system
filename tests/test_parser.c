#include "parser.h"
#include <unistd.h>

#define WRITE(s) write(1, s, sizeof(s) - 1)

#define GRN "\033[0;32m"
#define YLW "\033[1;33m"
#define BLU "\033[0;34m"
#define CYN "\033[0;36m"
#define BLD "\033[1m"
#define DIM "\033[2m"
#define RST "\033[0m"

int main() {
    WRITE("\n" BLU BLD "  ╔══════════════════════════════════════════════╗\n" RST);
    WRITE(     BLU BLD "  ║      testes do parser — SO Grupo 7           ║\n" RST);
    WRITE(     BLU BLD "  ╚══════════════════════════════════════════════╝\n" RST "\n");

    /* teste 1: comando simples */
    WRITE(CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(CYN "  │" RST "  " YLW BLD "teste 1: comando simples\n" RST);
    WRITE(CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t1[] = {"echo", "ola"};
    Pipeline *p1 = parse_pipeline(t1, 2);
    print_pipeline(p1);
    free_pipeline(p1);

    /* teste 2: redirecionamento de output */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 2: redirecionamento >\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t2[] = {"echo", "ola", ">", "out.txt"};
    Pipeline *p2 = parse_pipeline(t2, 4);
    print_pipeline(p2);
    free_pipeline(p2);

    /* teste 3: pipe entre dois comandos */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 3: pipe |\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t3[] = {"grep", "system", "/etc/passwd", "|", "wc", "-l"};
    Pipeline *p3 = parse_pipeline(t3, 6);
    print_pipeline(p3);
    free_pipeline(p3);

    /* teste 4: tudo junto */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 4: < | > combinados\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t4[] = {"grep", "system", "<", "in.txt", "|", "wc", "-l", ">", "out.txt"};
    Pipeline *p4 = parse_pipeline(t4, 9);
    print_pipeline(p4);
    free_pipeline(p4);

    WRITE("\n" GRN BLD "  ╔══════════════════════════════════════════════╗\n" RST);
    WRITE(     GRN BLD "  ║         TESTES CONCLUÍDOS!\n" RST);
    WRITE(     GRN BLD "  ╚══════════════════════════════════════════════╝\n" RST "\n");

    return 0;
}