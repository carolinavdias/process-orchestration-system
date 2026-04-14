CC = gcc 
CFLAGS = -Wall -g -Iinclude 
LDFLAGS = 

all: folders controller runner

controller: bin/controller

runner: bin/runner

folders:
	@mkdir -p src include obj bin tmp

bin/controller: obj/controller.o obj/utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

bin/runner: obj/runner.o
	$(CC) $^ -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f obj/* tmp/* bin/*

format:
	clang-format -i src/*.c include/*.h