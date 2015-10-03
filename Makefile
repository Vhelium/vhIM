CC = clang
CFLAGS = -Wall -g -pthreads -I/usr/include/gdsl -L/usr/lib -lgdsl -lssl -lcrypto

SRC_SERVER:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v client/))
SRC_CLIENT:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v server/))

server: $(SRC_SERVER)
	$(CC) $(CFLAGS) -o bin/server $(SRC_SERVER)

client: $(SRC_CLIENT)
	$(CC) $(CFLAGS) -o bin/client $(SRC_CLIENT)

test: src/test.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o bin/test test.c $(SRC_SERVER)

clean:
	$(rm) bin/server
	$(rm) bin/client
	$(rm) bin/test
