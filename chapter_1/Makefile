CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0 -D_POSIX_C_SOURCE=200809L

# Detect macOS vs Linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -D__APPLE__
endif

TARGET = compiler
OBJS = main.o lexer.o parser.o ast.o semantic.o tacky.o tacky_gen.o asm_ast.o codegen.o emit.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f *.i *.s

.PHONY: all clean
