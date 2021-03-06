# Makefile for vhIM project.

# Compiler and CFLAGS definitions.
CC=clang
CFLAGS=-Wall -g -pthreads -L/usr/lib -L/usr/local/lib -lssl -lcrypto -lpthread -lz -lm -ldl -lncurses -Wno-unused-function
# Additional flags for server compilation.
SERVER_FLAGS=-I/usr/include/gdsl -I/usr/local/include/gdsl -L/usr/lib -lgdsl -lmysqlclient -I/usr/include/mysql

SRC_SERVER:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v client/))
SRC_CLIENT:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v server/ | grep -v sql/))
SRC_TEST:=$(shell find src -type f -name *.c | grep -v server/ | grep -v client/) 

all: clean server client test

server: $(SRC_SERVER)
	$(CC) $(CFLAGS) $(SERVER_FLAGS) -o bin/server $(SRC_SERVER)

client: $(SRC_CLIENT)
	$(CC) $(CFLAGS) -o bin/client $(SRC_CLIENT)

test: $(SRC_TEST)
	$(CC) $(CFLAGS) $(SERVER_FLAGS) -o bin/test $(SRC_TEST)

clean:
	rm -rf bin/server
	rm -rf bin/client
	rm -rf bin/test
