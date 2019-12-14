CC = gcc
CFLAGS = -std=gnu11 --static


all: IPC-shmem
IPC-shmem: IPC-shmem.c 
	$(CC) $(CFLAGS) -o IPC-shmem IPC-shmem.c

IPC-pipe: IPC-pipe.c 
	$(CC) $(CFLAGS) -o IPC-pipe IPC-pipe.c

clean: 
	rm -f IPC-pipe
	rm -f IPC-shmem 
