CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

SRC = src/main.c \
      src/discord_gateway.c \
      src/commands.c \
      src/lavalink_client.c \
      src/queue.c

OBJ = $(SRC:.c=.o)
BIN = submusic

PKG_CFLAGS := $(shell pkg-config --cflags libwebsockets jansson libcurl)
PKG_LIBS := $(shell pkg-config --libs libwebsockets jansson libcurl)

CFLAGS += $(PKG_CFLAGS)
LDFLAGS += $(PKG_LIBS)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
