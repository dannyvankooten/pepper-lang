CFLAGS += -std=c11 -Wall
TESTFLAGS = $(CFLAGS) -g -DDEBUG
BINDIR := bin
DATE=$(shell date '+%Y-%m-%d')

all: monkey repl tests

repl: $(BINDIR)
	$(CC) $(CFLAGS) src/repl.c $(addprefix src/, eval.c parser.c env.c lexer.c token.c object.c) -ledit -Ofast -o $(BINDIR)/repl

monkey: $(BINDIR)
	$(CC) $(CFLAGS) src/monkey.c $(addprefix src/, eval.c parser.c env.c lexer.c token.c object.c) -Ofast --optimize -o $(BINDIR)/monkey 

tests: $(BINDIR) lexer_test parser_test eval_test 

lexer_test:
	$(CC) $(TESTFLAGS) src/lexer_test.c src/lexer.c src/token.c -o $(BINDIR)/lexer_test
	$(BINDIR)/lexer_test	

parser_test:
	$(CC) $(TESTFLAGS) src/parser_test.c src/parser.c src/lexer.c src/token.c -o $(BINDIR)/parser_test
	$(BINDIR)/parser_test

eval_test:
	$(CC) $(TESTFLAGS) src/eval_test.c $(addprefix src/, eval.c parser.c env.c lexer.c token.c object.c) -o $(BINDIR)/eval_test
	$(BINDIR)/eval_test

$(BINDIR):
	mkdir -p $(BINDIR)

bench: monkey
	echo "**$(shell date '+%Y-%m-%d %H:%M')** (fib 30)" >> benchmarks.md
	/usr/bin/time --append -o benchmarks.md ./bin/monkey fibonacci.monkey
	echo "" >> benchmarks.md

clean:
	rm -r $(BINDIR)

valgrind: 
	docker run -v $(PWD):/root/build -d -p 22021:22 messeb/valgrind	
	ssh -p 22021 root@localhost