CC     = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -Iinclude/scheduler
LDFLAGS =

all: folders controller runner

controller: bin/controller
runner:     bin/runner

folders:
	@mkdir -p src include obj bin tmp logs src/scheduler include/scheduler

bin/controller: obj/controller.o obj/scheduler.o obj/policy.o obj/queue.o obj/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/runner: obj/runner.o obj/utils.o obj/parser.o obj/executor.o
	$(CC) $^ -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/scheduler/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/* logs/*

format:
	clang-format -i src/*.c src/scheduler/*.c include/*.h include/scheduler/*.h