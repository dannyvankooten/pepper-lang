CFLAGS = -std=c11 -Wall -Werror
TESTFLAGS = $(CFLAGS) -g 
BINDIR := bin
DATE=$(shell date '+%Y-%m-%d')

all: monkey repl tests

repl: $(BINDIR)
	$(CC) $(CFLAGS) repl.c  -ledit -o $(BINDIR)/repl

monkey: $(BINDIR)
	$(CC) $(CFLAGS) monkey.c -o $(BINDIR)/monkey $(MAKEFLAGS)

monkey_release: $(BINDIR) 
	$(CC) $(CFLAGS) -O3 --optimize monkey.c -o $(BINDIR)/monkey $(MAKEFLAGS)

tests: $(BINDIR) lexer_test parser_test env_test eval_test 

lexer_test:
	$(CC) $(TESTFLAGS) lexer_test.c -o $(BINDIR)/lexer_test && $(BINDIR)/lexer_test	

parser_test:
	$(CC) $(TESTFLAGS) parser_test.c -o $(BINDIR)/parser_test && $(BINDIR)/parser_test

eval_test:
	$(CC) $(TESTFLAGS) eval_test.c -o $(BINDIR)/eval_test && $(BINDIR)/eval_test

env_test:
	$(CC) $(TESTFLAGS) env_test.c -o $(BINDIR)/env_test && $(BINDIR)/env_test

$(BINDIR):
	mkdir -p $(BINDIR)

bench: monkey_release 
	echo "**$(shell date '+%Y-%m-%d %H:%M')** (fib 30)" >> benchmarks.md
	/usr/bin/time --append -o benchmarks.md ./bin/monkey fibonacci.monkey
	echo "" >> benchmarks.md

clean:
	rm -r $(BINDIR)