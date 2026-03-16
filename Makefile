CC=gcc
CFLAGS=-Wall -Wextra -O2 -Isrc -Iinclude
LDFLAGS=-lconcord -lopus -lsodium -lcurl -lpthread

SRC=$(wildcard src/*.c)

all: submusic

submusic: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

clean:
	rm -f submusic

.PHONY: all clean
