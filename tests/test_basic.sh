#!/bin/bash
#testes básicos do runner e controller

PASS=0
FAIL=0

check() {
    if [ "$1" = "$2" ]; then
        echo "✓ $3"
        PASS=$((PASS + 1))
    else
        echo "✗ $3 (esperado: '$2', obtido: '$1')"
        FAIL=$((FAIL + 1))
    fi
}

echo "--- a correr testes ---"

#por agora só verifica que os binários existem
check "$(test -f bin/controller && echo ok)" "ok" "controller compilado"
check "$(test -f bin/runner && echo ok)" "ok" "runner compilado"

echo ""
echo "resultado: $PASS passou(aram), $FAIL falhou(aram)"

#já fica com a estrutura para irmos adicionando testes mais complexos à medida que o projeto cresce:)