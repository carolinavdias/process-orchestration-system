CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS =

SRC = src
OBJ = obj
BIN = bin

CONTROLLER_SRC = $(SRC)/controller.c
RUNNER_SRC = $(SRC)/runner.c

CONTROLLER_OBJ = $(OBJ)/controller.o
RUNNER_OBJ = $(OBJ)/runner.o

all: folders controller runner

folders:
	mkdir -p $(SRC) include $(OBJ) $(BIN) tmp

controller: $(BIN)/controller

runner: $(BIN)/runner

$(BIN)/controller: $(CONTROLLER_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(BIN)/runner: $(RUNNER_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)/* $(BIN)/* tmp/*