CC = gcc
CFLAGS = -lrt -lpthread

all: msg_thread

msg_thread: msg_thread.c
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f msg_thread
