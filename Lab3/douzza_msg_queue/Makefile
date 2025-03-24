CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

TARGET = msg_thread
SRC = msg_thread.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean