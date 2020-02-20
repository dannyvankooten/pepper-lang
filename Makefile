CFLAGS = -std=c99 -Wall -ledit
TESTFLAGS = $(CFLAGS) -g 
repl: .dist
	$(CC) $(CFLAGS) repl.c -o .dist/repl
	.dist/repl

test: .dist lexer_test parser_test


lexer_test:
	$(CC) $(TESTFLAGS) lexer_test.c -o .dist/lexer_test
	.dist/lexer_test || (echo "Test failed" && exit 1)	

parser_test:
	$(CC) $(TESTFLAGS) parser_test.c -o .dist/parser_test
	.dist/parser_test || (echo "Test failed" && exit 1)	

.dist:
	mkdir -p .dist

clean:
	rm -r .dist