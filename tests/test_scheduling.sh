#!/bin/bash
# test_scheduling.sh
# avaliação de políticas de escalonamento (fcfs vs rr)
# uso: ./tests/test_scheduling.sh

CONTROLLER=./bin/controller
RUNNER=./bin/runner
RESULTS=./logs/scheduling_results.txt

mkdir -p logs
> $RESULTS

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

SEP="${DIM}  ─────────────────────────────────────────────${RESET}"

log_file() { echo "$1" >> $RESULTS; }

print_header() {
    echo -e ""
    echo -e "${BOLD}${BLUE}  ╔══════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}  ║   AVALIAÇÃO DE POLÍTICAS DE ESCALONAMENTO    ║${RESET}"
    echo -e "${BOLD}${BLUE}  ║           SO — Grupo 7 · Maio 2026           ║${RESET}"
    echo -e "${BOLD}${BLUE}  ╚══════════════════════════════════════════════╝${RESET}"
    echo -e ""
}

print_test_header() {
    local policy=$(echo $1 | tr '[:lower:]' '[:upper:]')
    local parallel=$2
    echo -e ""
    echo -e "${BOLD}${CYAN}  ┌─────────────────────────────────────────────${RESET}"
    echo -e "${BOLD}${CYAN}  │${RESET}  ${YELLOW}${BOLD}política: $policy${RESET}   ${CYAN}|${RESET}   ${YELLOW}${BOLD}paralelo: $parallel${RESET}"
    echo -e "${BOLD}${CYAN}  │${RESET}  ${DIM}$(date '+%H:%M:%S — %d/%m/%Y')${RESET}"
    echo -e "${BOLD}${CYAN}  └─────────────────────────────────────────────${RESET}"
}

print_results() {
    local policy=$1
    local parallel=$2
    echo -e ""
    echo -e "${BOLD}  Resultados — ${policy} | paralelo ${parallel}${RESET}"
    echo -e "$SEP"
    while IFS= read -r line; do
        user=$(echo "$line" | grep -o 'user=[^ ]*' | cut -d= -f2)
        cmd=$(echo "$line"  | grep -o 'cmd=[^ ]*'  | cut -d= -f2)
        dur=$(echo "$line"  | grep -o 'duration=[^ ]*' | cut -d= -f2)
        echo -e "  ${DIM}│${RESET}  ${CYAN}user $user${RESET}   cmd=$cmd   ${BOLD}${GREEN}$dur${RESET}"
    done < logs/commands.log
    echo -e "$SEP"
    local last_dur
    last_dur=$(tail -1 logs/commands.log | grep -o 'duration=[0-9]*' | cut -d= -f2)
    echo -e "  ${BOLD}tempo total do último comando: ${YELLOW}${last_dur}ms${RESET}"
}

run_test() {
    local policy=$1
    local parallel=$2

    print_test_header "$policy" "$parallel"
    log_file ""
    log_file "=== política: $policy | paralelo: $parallel | $(date) ==="

    > logs/commands.log
    rm -f /tmp/controller_fifo /tmp/runner_fifo_*

    $CONTROLLER $parallel $policy >> $RESULTS 2>&1 &
    CTRL_PID=$!
    disown $CTRL_PID

    while [ ! -p /tmp/controller_fifo ]; do sleep 0.1; done

    echo -e "\n  ${DIM}a lançar runners em paralelo...${RESET}"
    echo -e "  ${DIM}│  user 1 → sleep 3${RESET}"
    echo -e "  ${DIM}│  user 2 → sleep 1${RESET}"
    echo -e "  ${DIM}│  user 3 → sleep 2${RESET}"
    echo -e "  ${DIM}│  user 4 → sleep 3${RESET}"
    echo -e "  ${DIM}│  user 1 → echo (2º comando)${RESET}"

    $RUNNER -e 1 "sleep 3"                    > /dev/null 2>&1 & R1=$!
    $RUNNER -e 2 "sleep 1"                    > /dev/null 2>&1 & R2=$!
    $RUNNER -e 3 "sleep 2"                    > /dev/null 2>&1 & R3=$!
    $RUNNER -e 4 "sleep 3"                    > /dev/null 2>&1 & R4=$!
    $RUNNER -e 1 "echo user1_segundo_comando" > /dev/null 2>&1 & R5=$!

    wait $R1 $R2 $R3 $R4 $R5

    $RUNNER -s > /dev/null 2>&1
    wait $CTRL_PID 2>/dev/null
    sleep 0.5

    print_results "$policy" "$parallel"
    log_file "--- resultados ---"
    cat logs/commands.log >> $RESULTS

    echo -e "\n  ${GREEN}✓ teste concluído${RESET}"
}

# ── main ────────────────────────────────────────────────────────────
print_header

run_test "fcfs" 1
run_test "rr"   1
run_test "fcfs" 2
run_test "rr"   2

echo -e ""
echo -e "${BOLD}${GREEN}  ╔══════════════════════════════════════════════╗${RESET}"
echo -e "${BOLD}${GREEN}  ║         TODOS OS TESTES CONCLUÍDOS!          ║${RESET}"
echo -e "${BOLD}${GREEN}  ║   Resultados em logs/scheduling_results.txt  ║${RESET}"
echo -e "${BOLD}${GREEN}  ╚══════════════════════════════════════════════╝${RESET}"
echo -e ""