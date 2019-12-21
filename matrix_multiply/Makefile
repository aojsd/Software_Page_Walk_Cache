CC = gcc
CFLAGS = -std=gnu11 --static


all: matrix
matrix: matrix-mult.c 
	$(CC) $(CFLAGS) -o matrix-mult matrix-mult.c

clean: 
	rm -f matrix-mult
