#!/bin/bash
# test_basic.sh: testes de funcionamento do sistema
# uso: ./tests/test_basic.sh

CONTROLLER=./bin/controller
RUNNER=./bin/runner
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'
SEP="${DIM}  ─────────────────────────────────────────────${RESET}"

check() {
    if [ "$1" = "$2" ]; then
        echo -e "  ${GREEN}✓${RESET}  $3"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗${RESET}  $3"
        echo -e "     ${DIM}esperado: '$2'   obtido: '$1'${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

cleanup() {
    pkill -f "$CONTROLLER" 2>/dev/null
    pkill -f "$RUNNER"     2>/dev/null
    rm -f /tmp/controller_fifo /tmp/runner_fifo_*
    sleep 0.3
}

start_controller() {
    cleanup
    $CONTROLLER 2 fcfs > /dev/null 2>&1 &
    CTRL_PID=$!
    disown $CTRL_PID
    while [ ! -p /tmp/controller_fifo ]; do sleep 0.1; done
}

# cabeçalho
echo -e ""
echo -e "${BOLD}${BLUE}  ╔══════════════════════════════════════════════╗${RESET}"
echo -e "${BOLD}${BLUE}  ║        TESTES BÁSICOS — SO Grupo 7           ║${RESET}"
echo -e "${BOLD}${BLUE}  ║           SO — Grupo 7 · Maio 2026           ║${RESET}"
echo -e "${BOLD}${BLUE}  ╚══════════════════════════════════════════════╝${RESET}"
echo -e ""

# 1. compilação
echo -e "${BOLD}${CYAN}  ┌─────────────────────────────────────────────${RESET}"
echo -e "${BOLD}${CYAN}  │${RESET}  ${YELLOW}${BOLD}1. compilação${RESET}"
echo -e "${BOLD}${CYAN}  └─────────────────────────────────────────────${RESET}"
echo -e ""

check "$(test -f bin/controller && echo ok)" "ok" "bin/controller existe"
check "$(test -f bin/runner     && echo ok)" "ok" "bin/runner existe"

# 2. modos do runner
echo -e ""
echo -e "${BOLD}${CYAN}  ┌─────────────────────────────────────────────${RESET}"
echo -e "${BOLD}${CYAN}  │${RESET}  ${YELLOW}${BOLD}2. modos do runner${RESET}"
echo -e "${BOLD}${CYAN}  └─────────────────────────────────────────────${RESET}"
echo -e ""

start_controller

# -e básico
out=$($RUNNER -e 1 "echo hello" 2>&1)
check "$(echo "$out" | grep -c 'submitted')" "1" "modo -e: notifica submissão"
check "$(echo "$out" | grep -c 'finished')"  "1" "modo -e: comando simples executa e termina"

# operadores
out=$($RUNNER -e 1 "grep root /etc/passwd | wc -l" 2>&1)
check "$(echo "$out" | grep -c 'finished')" "1" "modo -e: pipeline com | executa"

rm -f /tmp/so_test_out.txt
$RUNNER -e 1 "echo teste_redir > /tmp/so_test_out.txt" > /dev/null 2>&1
check "$(cat /tmp/so_test_out.txt 2>/dev/null)" "teste_redir" "modo -e: redirecionamento > funciona"
rm -f /tmp/so_test_out.txt

echo "linha_teste" > /tmp/so_test_in.txt
out=$($RUNNER -e 1 "wc -l < /tmp/so_test_in.txt" 2>&1)
check "$(echo "$out" | grep -c 'finished')" "1" "modo -e: redirecionamento < funciona"
rm -f /tmp/so_test_in.txt

$RUNNER -e 1 "ls ficheiro_inexistente 2> /tmp/so_test_err.txt" > /dev/null 2>&1
check "$(test -s /tmp/so_test_err.txt && echo ok)" "ok" "modo -e: redirecionamento 2> funciona"
rm -f /tmp/so_test_err.txt

# -c
out=$($RUNNER -c 2>&1)
check "$(echo "$out" | grep -c 'Executing')" "1" "modo -c: retorna lista de comandos"

# 3. log persistente
echo -e ""
echo -e "${BOLD}${CYAN}  ┌─────────────────────────────────────────────${RESET}"
echo -e "${BOLD}${CYAN}  │${RESET}  ${YELLOW}${BOLD}3. registo persistente${RESET}"
echo -e "${BOLD}${CYAN}  └─────────────────────────────────────────────${RESET}"
echo -e ""

check "$(test -f logs/commands.log && echo ok)" "ok" "logs/commands.log criado"
check "$(grep 'user=' logs/commands.log 2>/dev/null | head -1 | grep -c 'cmd=')" "1" "log contém user_id e cmd_id"
ENTRIES=$(grep -c 'duration=' logs/commands.log 2>/dev/null || echo 0)
check "$([ "$ENTRIES" -gt 0 ] && echo ok || echo fail)" "ok" "log contém entradas com duração"

# 4. shutdown
echo -e ""
echo -e "${BOLD}${CYAN}  ┌─────────────────────────────────────────────${RESET}"
echo -e "${BOLD}${CYAN}  │${RESET}  ${YELLOW}${BOLD}4. shutdown${RESET}"
echo -e "${BOLD}${CYAN}  └─────────────────────────────────────────────${RESET}"
echo -e ""

out=$($RUNNER -s 2>&1)
check "$(echo "$out" | grep -c 'exited')" "1" "modo -s: controller termina correctamente"
sleep 0.3
check "$(test ! -p /tmp/controller_fifo && echo ok)" "ok" "shutdown remove FIFO global"

# resultado
cleanup
TOTAL=$((PASS + FAIL))
echo -e ""
echo -e "$SEP"

if [ $FAIL -eq 0 ]; then
    echo -e ""
    echo -e "${BOLD}${GREEN}  ╔══════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${GREEN}  ║   TODOS OS TESTES PASSARAM! ($PASS/$TOTAL)          ║${RESET}"
    echo -e "${BOLD}${GREEN}  ╚══════════════════════════════════════════════╝${RESET}"
else
    echo -e ""
    echo -e "${BOLD}${RED}  ╔══════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${RED}  ║  RESULTADO: ${GREEN}$PASS passaram${RED} | ${RED}$FAIL falharam${RED}            ║${RESET}"
    echo -e "${BOLD}${RED}  ╚══════════════════════════════════════════════╝${RESET}"
fi
echo -e ""