all: server client

server: server.c
	gcc server.c -g -lncurses -o server

client: client.c
	gcc client.c -g -lncurses -o client

clean:
	rm -f server client log.txt
