CFLAGS+= -std=c11 -Wall -Isrc/
TESTFLAGS= $(CFLAGS) -g -DDEBUG
LIBS= -ledit
DATE=$(shell date '+%Y-%m-%d')

LEXER_SRC= src/lexer.c src/token.c
PARSER_SRC= src/parser.c $(LEXER_SRC)
EVAL_SRC= src/eval.c src/object.c src/env.c src/builtins.c $(PARSER_SRC)
COMPILER_SRC= src/opcode.c src/compiler.c src/object.c src/symbol_table.c $(PARSER_SRC)
VM_SRC= src/vm.c src/opcode.c src/object.c src/symbol_table.c $(PARSER_SRC)

all: bin/ bin/monkey bin/repl
tests: bin/ bin/lexer_test bin/parser_test bin/eval_test bin/compiler_test bin/vm_test bin/symbol_table_test
	
bin/:
	mkdir -p bin/

bin/repl: src/repl.c $(EVAL_SRC) src/vm.c src/opcode.c src/symbol_table.c src/compiler.c
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

bin/monkey: src/monkey.c $(EVAL_SRC)
	$(CC) $(CFLAGS) $^ -Ofast -finline-limit=1024 -DNDEBUG -o $@

bin/lexer_test: tests/lexer_test.c $(LEXER_SRC) 
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/parser_test: tests/parser_test.c $(PARSER_SRC)
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/eval_test: tests/eval_test.c $(EVAL_SRC)
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/opcode_test: tests/opcode_test.c src/opcode.c
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/compiler_test: tests/compiler_test.c $(COMPILER_SRC) 
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/vm_test: tests/vm_test.c $(VM_SRC) src/compiler.c
	$(CC) $(TESTFLAGS) $^ -o $@ && $@

bin/symbol_table_test: tests/symbol_table_test.c src/symbol_table.c 
	$(CC) $(TESTFLAGS) $^ -o $@ && $@	

.PHONY: bench
bench: bin/monkey
	echo "**$(shell date '+%Y-%m-%d %H:%M')** (fib 35)" >> benchmarks.md
	/usr/bin/time --append -o benchmarks.md ./bin/monkey fibonacci.monkey
	echo "" >> benchmarks.md

.PHONY: clean
clean:
	rm -r bin

.PHONY: watch
watch:
	find src/ | entr -s 'make tests'

.PHONY: valgrind
valgrind: 
	docker run -v $(PWD):/root/build -d -p 22021:22 messeb/valgrind	
	ssh -p 22021 root@localhost