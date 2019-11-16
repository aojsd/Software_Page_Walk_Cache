CC = gcc
CFLAGS = -std=gnu11 --static

all: pagefault

pagefault: pagefault-test.c
	$(CC) pagefault-test.c -o pagefault-test $(CFLAGS)

clean:
	rm -f pagefault-test