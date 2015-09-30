server: server/server.c crypto.c datapacket.c
	clang -g -Wall -o bin/server server/server.c crypto.c datapacket.c

client: client/client.c crypto.c datapacket.c
	clang -g -Wall -o bin/client client/client.c crypto.c datapacket.c

test: test.c datapacket.c crypto.c
	clang -g -Wall -o bin/test test.c datapacket.c crypto.c

clean:
	$(rm) bin/server
