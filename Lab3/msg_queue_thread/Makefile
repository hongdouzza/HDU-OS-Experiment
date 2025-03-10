CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu11
LDFLAGS = -lrt -lpthread

all: msg_thread

msg_thread: msg_thread.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f msg_thread
