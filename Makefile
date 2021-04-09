CFLAGS+= -Werror -Wall -Isrc/ -g 
LDLIBS=-l edit
DATE=$(shell date '+%Y-%m-%d')
VPATH= src
LEXER_SRC= lexer.c token.c
PARSER_SRC= parser.c $(LEXER_SRC)
EVAL_SRC= eval.c object.c env.c builtins.c opcode.c $(PARSER_SRC)
COMPILER_SRC= compiler.c object.c symbol_table.c opcode.c $(PARSER_SRC)
VM_SRC= vm.c opcode.c object.c symbol_table.c builtins.c $(PARSER_SRC)
PREFIX= /usr/local
TESTS= bin/lexer_test bin/parser_test bin/opcode_test bin/eval_test bin/compiler_test bin/vm_test bin/symbol_table_test 

ifeq "$(CC)" "gcc"
    CFLAGS+=  -fno-gcse -fno-crossjumping
endif

all: bin/monkey 

bin/:
	mkdir -p bin/

bin/monkey: monkey.c $(EVAL_SRC) vm.c opcode.c symbol_table.c compiler.c | bin/
	$(CC) $(CFLAGS) $^ -Ofast -march=native -DNDEBUG -DOPT_AGGRESIVE  -o $@ $(LDLIBS)

# tests
bin/lexer_test: tests/lexer_test.c $(LEXER_SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/parser_test: tests/parser_test.c $(PARSER_SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/eval_test: tests/eval_test.c $(EVAL_SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/opcode_test: tests/opcode_test.c opcode.c | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/compiler_test: tests/compiler_test.c $(COMPILER_SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/vm_test: tests/vm_test.c $(VM_SRC) compiler.c | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
bin/symbol_table_test: tests/symbol_table_test.c symbol_table.c | bin/
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

check: $(TESTS)
	for test in $^; do $$test || exit 1; done

memcheck: $(TESTS)
	for test in $^; do valgrind $$test || exit 1; done

.PHONY: clean
clean:
	rm -r bin