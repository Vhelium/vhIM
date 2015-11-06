# Makefile for vhIM project.

# Compiler and CFLAGS definitions.
CC=clang
CFLAGS=-Wall -g -L/usr/lib -L/usr/local/lib -lssl -lcrypto -lpthread -lz -lm -ldl -lncurses -Wno-unused-function
# Additional flags for server compilation.
SERVER_FLAGS=-I/usr/include/gdsl -I/usr/local/include/gdsl -L/usr/lib -lgdsl -lmysqlclient -I/usr/include/mysql -pthreads
GTKLIB=`pkg-config --libs --cflags gtk+-3.0`

SRC_SERVER:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v client/))
SRC_CLIENT:=$(filter-out src/test.c, $(shell find src -type f -name *.c | grep -v server/ | grep -v sql/))
SRC_TEST:=$(shell find src -type f -name *.c | grep -v server/ | grep -v client/) 

all: clean make_resources server client

make_resources:
	glib-compile-resources src/client/client_ui_gui.gresource.xml \
		--target=src/client/resources.c --sourcedir=src/client/ \
		--generate-source

server: $(SRC_SERVER)
	$(CC) $(CFLAGS) $(SERVER_FLAGS) -o bin/server $(SRC_SERVER)

client: $(SRC_CLIENT)
	$(CC) $(CFLAGS) -o bin/client $(SRC_CLIENT) $(GTKLIB)

test: $(SRC_TEST)
	$(CC) $(CFLAGS) $(SERVER_FLAGS) -o bin/test $(SRC_TEST)

clean:
	rm -rf src/client/resources.c
	rm -rf bin/server
	rm -rf bin/client
	rm -rf bin/test
