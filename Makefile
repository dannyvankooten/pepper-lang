CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -Werror -Wshadow

test: *.c *.h .dist
	$(CC) $(CFLAGS) *.c -o .dist/test
	.dist/test || (echo "Test failed" && exit 1)

debug: *.c *.h  .dist
	$(CC) $(CFLAGS) -g *.c -o .dist/debug

.dist:
	mkdir -p dist

clean:
	rm -r .dist