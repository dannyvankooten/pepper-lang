CC = gcc
CFLAGS = -std=c99 -Wall -ledit

repl: .dist
	$(CC) $(CFLAGS) repl.c -o .dist/repl
	.dist/repl

.dist:
	mkdir -p .dist

clean:
	rm -r .dist