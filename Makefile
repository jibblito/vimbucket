all: server client

server: server.c vimline.c
	gcc server.c vimline.c -g -lncurses -o server

client: client.c vimline.c
	gcc client.c vimline.c -g -lncurses -o client


clean:
	rm -f server client log.txt
