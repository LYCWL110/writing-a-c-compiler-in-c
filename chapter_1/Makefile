CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0 -D_POSIX_C_SOURCE=200809L

# Detect macOS vs Linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -D__APPLE__
endif

TARGET = compiler
OBJS = main.o lexer.o parser.o ast.o asm_ast.o codegen.o emit.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f *.i *.s

test:
	@echo "Building compiler..."
	@$(MAKE) --no-print-directory $(TARGET)
	@echo "Running test suite..."
	/home/lycwl/code/writing-a-c-compiler-tests-main./writing-a-c-compiler-tests-main/writing-a-c-compiler-tests-main/test_compiler ./$(TARGET) --chapter 1 --expected-error-codes 1

test-lex:
	@$(MAKE) --no-print-directory $(TARGET)
	/home/lycwl/code/writing-a-c-compiler-tests-main./writing-a-c-compiler-tests-main/writing-a-c-compiler-tests-main/test_compiler ./$(TARGET) --chapter 1 --stage lex

test-parse:
	@$(MAKE) --no-print-directory $(TARGET)
	/home/lycwl/code/writing-a-c-compiler-tests-main./writing-a-c-compiler-tests-main/writing-a-c-compiler-tests-main/test_compiler ./$(TARGET) --chapter 1 --stage parse

test-codegen:
	@$(MAKE) --no-print-directory $(TARGET)
	/home/lycwl/code/writing-a-c-compiler-tests-main./writing-a-c-compiler-tests-main/writing-a-c-compiler-tests-main/test_compiler ./$(TARGET) --chapter 1 --stage codegen

.PHONY: all clean test test-lex test-parse test-codegen
