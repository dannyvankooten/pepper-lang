CFLAGS = -std=c99 -Wall
TESTFLAGS = $(CFLAGS) -g 
BINDIR := bin

all: monkey repl tests

repl: $(BINDIR)
	$(CC) $(CFLAGS) repl.c  -ledit -o $(BINDIR)/repl

monkey: $(BINDIR)
	$(CC) $(CFLAGS) monkey.c -o $(BINDIR)/monkey $(MAKEFLAGS)

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

clean:
	rm -r $(BINDIR)