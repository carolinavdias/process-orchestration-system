<div align="center">

# 🖥️ ᴏʀϙᴜᴇꜱᴛʀᴀçãᴏ ᴍᴜʟᴛɪ-ʀᴜɴɴᴇʀ

**Sistemas Operativos · Universidade do Minho · 2025/2026**

![Language](https://img.shields.io/badge/linguagem-C-blue?style=flat-square)
![Platform](https://img.shields.io/badge/plataforma-Linux-orange?style=flat-square)
![IPC](https://img.shields.io/badge/IPC-Named%20Pipes-green?style=flat-square)

*Orquestrador de comandos bash multi-utilizador com named pipes, pipelines e escalonamento configurável.*

</div>

---

## 👥 Grupo 7

| Nome | Número |
|---|---|
| Ana Patrícia da Silva Machado | a111661 |
| Carolina Francisca Vieira Dias | a111179 |
| Gonçalo Nuno Gomes Pombal | a110681 |

---

## ⚙️ Compilar

```bash
make        # compila controller, runner e testes
make clean  # remove binários e objectos
```

---

## 🚀 Utilização

**1. Arrancar o controller**
```bash
./bin/controller <parallel-commands> <sched-policy>

./bin/controller 1 fcfs   # sequencial, FCFS
./bin/controller 2 rr     # 2 slots paralelos, Round Robin
```

**2. Submeter um comando**
```bash
./bin/runner -e <user-id> "<comando>"

./bin/runner -e 1 "echo hello"
./bin/runner -e 1 "grep root /etc/passwd | wc -l"
./bin/runner -e 2 "ls -la > /tmp/saida.txt"
```

**3. Consultar estado**
```bash
./bin/runner -c
```

**4. Terminar o controller**
```bash
./bin/runner -s
```

---

## 🧪 Testes

```bash
./tests/test_basic.sh        # testes de funcionamento (14/14)
./bin/test_parser             # testes do módulo de parsing
./bin/test_executor           # testes do módulo de execução
./tests/test_scheduling.sh   # avaliação de políticas (~40s)
```

---

## 📁 Estrutura

```
.
├── src/
│   ├── controller.c          # servidor: receção, escalonamento, log
│   ├── runner.c              # cliente: submissão, execução, shutdown
│   ├── executor/
│   │   ├── parser.c          # parsing de pipelines e operadores
│   │   └── executor.c        # execução com fork, execvp e dup2
│   └── scheduler/
│       ├── scheduler.c       # lógica central de escalonamento
│       ├── policy.c          # políticas FCFS e Round Robin
│       └── queue.c           # filas por utilizador
├── include/                  # headers
├── tests/                    # scripts e programas de teste
├── logs/                     # registo persistente de comandos
└── Makefile
```

---

## ✅ Funcionalidades

| Funcionalidade | Estado |
|---|---|
| Modos `-e`, `-c`, `-s` | ✅ |
| Política FCFS | ✅ |
| Política Round Robin | ✅ |
| Execução paralela configurável | ✅ |
| Operadores `\|` `>` `2>` `<` | ✅ |
| Registo persistente em `logs/commands.log` | ✅ |
| Shutdown gracioso | ✅ |

---

<div align="center">

*Desenvolvido para a UC de Sistemas Operativos - LEI, Universidade do Minho · Grupo 7 · 2025/2026*

</div>
