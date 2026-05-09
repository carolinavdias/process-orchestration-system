#include "parser.h"
#include "executor.h"
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
    WRITE(     BLU BLD "  ║      testes do executor — SO Grupo 7         ║\n" RST);
    WRITE(     BLU BLD "  ╚══════════════════════════════════════════════╝\n" RST "\n");

    /* teste 1: comando simples */
    WRITE(CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(CYN "  │" RST "  " YLW BLD "teste 1: echo simples\n" RST);
    WRITE(CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t1[] = {"echo", "ola mundo"};
    Pipeline *p1 = parse_pipeline(t1, 2);
    execute_pipeline(p1);
    free_pipeline(p1);

    /* teste 2: redirecionamento de output */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 2: echo > ficheiro\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t2[] = {"echo", "hello", ">", "/tmp/test_out.txt"};
    Pipeline *p2 = parse_pipeline(t2, 4);
    execute_pipeline(p2);
    free_pipeline(p2);
    /* lê o ficheiro para confirmar */
    WRITE(DIM "  conteúdo de /tmp/test_out.txt:\n" RST);
    char *t2b[] = {"cat", "/tmp/test_out.txt"};
    Pipeline *p2b = parse_pipeline(t2b, 2);
    execute_pipeline(p2b);
    free_pipeline(p2b);

    /* teste 3: pipe entre dois comandos */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 3: grep | wc -l\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t3[] = {"grep", "root", "/etc/passwd", "|", "wc", "-l"};
    Pipeline *p3 = parse_pipeline(t3, 6);
    execute_pipeline(p3);
    free_pipeline(p3);

    /* teste 4: input de ficheiro */
    WRITE("\n" CYN "  ┌─────────────────────────────────────────────\n" RST);
    WRITE(    CYN "  │" RST "  " YLW BLD "teste 4: wc -l < ficheiro\n" RST);
    WRITE(    CYN "  └─────────────────────────────────────────────\n" RST "\n");
    char *t4[] = {"wc", "-l", "<", "/etc/passwd"};
    Pipeline *p4 = parse_pipeline(t4, 4);
    execute_pipeline(p4);
    free_pipeline(p4);

    WRITE("\n" GRN BLD "  ╔══════════════════════════════════════════════╗\n" RST);
    WRITE(     GRN BLD "  ║         TESTES CONCLUÍDOS!\n" RST);
    WRITE(     GRN BLD "  ╚══════════════════════════════════════════════╝\n" RST "\n");

    return 0;
}