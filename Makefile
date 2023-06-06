all: server client

server: server.c screen vimline
	gcc server.c vimline.o screen.o -lncurses -g -o server

client: client.c
	gcc client.c -g -lncurses -o client

screen: screen.c
	gcc screen.c -g -c -lncurses -o screen.o

vimline: vimline.c
	gcc vimline.c -g -c -o vimline.o


clean:
	rm -f server client log.txt ./*.o
