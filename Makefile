override CFLAGS+= -std=c11 -Wall -Isrc/ -g
LDLIBS= -ledit
DATE=$(shell date '+%Y-%m-%d')
VPATH = src
LEXER_SRC= lexer.c token.c
PARSER_SRC= parser.c $(LEXER_SRC)
EVAL_SRC= eval.c object.c env.c builtins.c opcode.c $(PARSER_SRC)
COMPILER_SRC= compiler.c object.c symbol_table.c opcode.c $(PARSER_SRC)
VM_SRC= vm.c opcode.c object.c symbol_table.c $(PARSER_SRC)
PREFIX = /usr/local

ifeq "$(CC)" "gcc"
	CFLAGS += -fno-crossjumping
endif

all: bin/monkey 

bin/:
	mkdir -p bin/

bin/monkey: monkey.c $(EVAL_SRC) vm.c opcode.c symbol_table.c compiler.c | bin/
	$(CC) $(CFLAGS) $^ -O3 -DOPT_AGGRESSIVE -DUNSAFE -DNDEBUG -o $@ $(LDLIBS)

bin/lexer_test: tests/lexer_test.c $(LEXER_SRC) | bin/
bin/parser_test: tests/parser_test.c $(PARSER_SRC) | bin/
bin/eval_test: tests/eval_test.c $(EVAL_SRC) | bin/
bin/opcode_test: tests/opcode_test.c opcode.c | bin/
bin/compiler_test: tests/compiler_test.c $(COMPILER_SRC) | bin/
bin/vm_test: tests/vm_test.c $(VM_SRC) compiler.c | bin/
bin/symbol_table_test: tests/symbol_table_test.c symbol_table.c | bin/

check: bin/lexer_test bin/parser_test bin/opcode_test bin/eval_test bin/compiler_test bin/vm_test bin/symbol_table_test 
	for test in $^; do $$test || exit 1; done

.PHONY: bench
bench: bin/monkey
	echo "**$(shell date '+%Y-%m-%d %H:%M')** (fib 35)" >> benchmarks.md
	/usr/bin/time --append -o benchmarks.md ./bin/monkey fibonacci.monkey
	echo "" >> benchmarks.md

.PHONY: install
install: bin/monkey
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/monkey

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/monkey

.PHONY: clean
clean:
	rm -r bin

.PHONY: valgrind
valgrind: 
	docker run -v $(PWD):/root/build -d -p 22021:22 messeb/valgrind	
	ssh -p 22021 root@localhost