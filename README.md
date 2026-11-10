<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/icon-dark.svg">
  <img src="assets/icon-light.svg" height="110" alt="">
</picture>

<h1>𝙋𝙧𝙤𝙘𝙚𝙨𝙨 𝙊𝙧𝙘𝙝𝙚𝙨𝙩𝙧𝙖𝙩𝙞𝙤𝙣 𝙎𝙮𝙨𝙩𝙚𝙢</h1>
<h3>Multi-Runner Command Execution over Named Pipes</h3>

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Bash](https://img.shields.io/badge/Bash-4EAA25?style=for-the-badge&logo=gnubash&logoColor=white)

*Academic project developed for **Sistemas Operativos** @ University of Minho*

</div>

---

## 🖥️ About

For Operating Systems we had to build, in C and using system calls, a small system that runs bash commands for several users at the same time. It has two programs: the `controller`, which receives every request and decides what runs next, and the `runner`, which each user launches to send a command, check what's going on or shut everything down.

The catch? They can only talk to each other through **named pipes**. No sockets, no shared memory, just FIFOs. That made us think a lot about who opens what, when, and who ends up stuck waiting for whom.

---

## 🔁 How it works

```
   runner (user 1) ─┐
   runner (user 2) ─┼──►  /tmp/controller_fifo  ──►  controller
   runner (user 3) ─┘                                  │  queue + policy
         ▲                                             │
         └────────────  /tmp/runner_fifo_<pid>  ◄──────┘
```

Every runner sends its request through the controller's FIFO and opens its own private one, named after its PID, to get the answer back. A new command goes to the queue first (`RESP_SUBMITTED`) and only starts when the controller gives the green light (`RESP_RUN`). Then the runner forks, runs it with `execvp` and tells the controller it's done, so the next one in line can go.

---

## ⚡ What it does

- runs commands with pipelines and redirection (`|`, `>`, `2>`, `<`)
- lets you choose how many commands run at the same time
- schedules with **FCFS** or **Round Robin**, with one queue per user
- shows what's running and what's still waiting (`-c`)
- keeps a log of every finished command with its user, id and duration
- shuts down gracefully, only after everything that was running finishes

---

## 🧩 My part

I was in charge of the extra features: the redirection and pipe operators, the scheduling policies, the test scripts and the Makefile. I also wrote a good chunk of the report.

---

## 😵 The tricky part

Honestly, the hardest part wasn't writing the features, it was getting the processes to wait for each other the right way. Making sure a runner doesn't start before its turn, opening the global FIFO so it doesn't block when no runner is around, and making the controller wait for everything to finish before closing took way more time than the features themselves.

My favourite bug: the status mode froze forever because a FIFO was opened with `O_RDONLY`, which blocks until someone opens the other end... and that someone was waiting for us. Switching to `O_RDWR` fixed it.

---

## 🚀 How to Run

```bash
# Compile
make

# Start the controller: <parallel-commands> <policy>
./bin/controller 1 fcfs     # one at a time, FCFS
./bin/controller 2 rr       # two at a time, Round Robin

# In another terminal, send some commands
./bin/runner -e 1 "echo hello"
./bin/runner -e 1 "grep root /etc/passwd | wc -l"
./bin/runner -e 2 "ls -la > /tmp/out.txt"

# See what's running and what's waiting
./bin/runner -c

# Shut it down
./bin/runner -s
```

---

## 🧪 Tests

```bash
./tests/test_basic.sh         # functional tests (14/14)
./bin/test_parser             # parser module
./bin/test_executor           # executor module
./tests/test_scheduling.sh    # FCFS vs RR with 1 and 2 slots (about 40s)
```

The scheduling test saves its results in `logs/scheduling_results.txt`.

---

## 🗂️ Project Structure

```
.
├── src/
│   ├── controller.c         # server: requests, scheduling, log
│   ├── runner.c             # client: submit, execute, status, shutdown
│   ├── utils.c              # shared helpers
│   ├── executor/
│   │   ├── parser.c         # pipelines and redirection operators
│   │   └── executor.c       # fork, execvp and dup2
│   └── scheduler/
│       ├── scheduler.c      # core scheduling logic
│       ├── policy.c         # FCFS and Round Robin
│       └── queue.c          # one queue per user
├── include/                 # header files
├── tests/                   # test scripts and programs
├── logs/                    # command log (created at runtime)
├── Relatorio_SO_Grupo07.pdf # project report (in Portuguese)
└── Makefile
```

---

## 🛠️ Tech Stack

`C` · `Named Pipes (FIFOs)` · `fork / execvp / waitpid` · `dup2` · `FCFS` · `Round Robin` · `Make` · `Bash`

---

## 👩‍💻 Authors

**Carolina Dias** · [@carolinavdias](https://github.com/carolinavdias)  
**Ana Patrícia Machado** · [@apatriciamachado](https://github.com/apatriciamachado)  
**Gonçalo Pombal** · [@pombas16](https://github.com/pombas16)

<p align="center"><i>built with way too many terminal windows open at the same time</i></p>
