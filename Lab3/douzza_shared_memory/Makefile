CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu11 -O2
LDFLAGS =
TARGETS = writer reader

.PHONY: all clean

all: $(TARGETS)

writer: writer.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

reader: reader.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o
