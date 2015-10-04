CC = clang
CFLAGS = -Wall -g -pthreads -I/usr/include/gdsl -L/usr/lib -lgdsl -lssl -lcrypto -lmysqlclient -lpthread -lz -lm -lssl -lcrypto -ldl -I/usr/include/mysql -I/usr/include/mysql

SRC_SERVER:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v client/))
SRC_CLIENT:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v server/))
SRC_TEST:=$(shell find src -type f -name *.c | grep -v server/ | grep -v client/) 

server: $(SRC_SERVER)
	$(CC) $(CFLAGS) -o bin/server $(SRC_SERVER)

client: $(SRC_CLIENT)
	$(CC) $(CFLAGS) -o bin/client $(SRC_CLIENT)

test: $(SRC_TEST)
	$(CC) $(CFLAGS) -o bin/test $(SRC_TEST)

clean:
	$(rm) bin/server
	$(rm) bin/client
	$(rm) bin/test
