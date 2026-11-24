CC      := gcc
CFLAGS  := -Wall -Wextra -O2
LDFLAGS := -pthread

SERVER_SRC := server/tcp_server.c
CLIENT_SRC := client/tcp_client.c

SERVER_BIN := tcp_server
CLIENT_BIN := tcp_client

.PHONY: all clean rebuild

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

rebuild: clean all