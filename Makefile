CC     = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -Iinclude/scheduler -Iinclude/executor
LDFLAGS =

all: folders controller runner

controller: bin/controller
runner:     bin/runner

folders:
	@mkdir -p src include obj bin tmp logs src/scheduler include/scheduler src/executor include/executor

bin/controller: obj/controller.o obj/scheduler.o obj/policy.o obj/queue.o obj/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/runner: obj/runner.o obj/parser.o obj/executor.o obj/utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/scheduler/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/executor/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/* logs/*

format:
	clang-format -i src/*.c src/scheduler/*.c include/*.h include/scheduler/*.h