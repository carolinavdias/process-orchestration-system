#!/bin/bash
# test_scheduling.sh
# testa e compara políticas de escalonamento (fcfs vs rr)
# uso: ./tests/test_scheduling.sh

CONTROLLER=./bin/controller
RUNNER=./bin/runner
RESULTS=./logs/scheduling_results.txt

mkdir -p logs
> $RESULTS

log() { echo "$1" | tee -a $RESULTS; }

run_test() {
    local policy=$1
    local parallel=$2

    log ""
    log "================================================"
    log "  política: $policy | paralelo: $parallel"
    log "  $(date)"
    log "================================================"

    # limpa log de comandos anterior
    > logs/commands.log
    rm -f /tmp/controller_fifo

    # arranca controller em background
    $CONTROLLER $parallel $policy >> $RESULTS 2>&1 &
    CTRL_PID=$!

    # espera que o FIFO exista antes de lançar runners
    while [ ! -p /tmp/controller_fifo ]; do sleep 0.1; done

    # lança runners com comandos variados em paralelo
    $RUNNER -e 1 "sleep 3" &
    $RUNNER -e 2 "sleep 1" &
    $RUNNER -e 3 "sleep 2" &
    $RUNNER -e 4 "sleep 3" &
    $RUNNER -e 1 "echo user1_segundo_comando" &

    # espera todos os runners
    wait

    # shutdown gracioso
    $RUNNER -s
    wait $CTRL_PID 2>/dev/null

    log ""
    log "--- resultados do log ---"
    cat logs/commands.log | tee -a $RESULTS
}

log "TESTES DE POLÍTICAS DE ESCALONAMENTO"
log ""

run_test "fcfs" 1
run_test "rr"   1
run_test "fcfs" 2
run_test "rr"   2

log ""
log "testes concluídos! resultados em $RESULTS"