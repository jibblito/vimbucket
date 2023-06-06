// Client side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <curses.h>
#define PORT 25565

int main(int argc, char const* argv[])
{
  initscr();
  cbreak();
  int status, valread, client_fd;
  struct sockaddr_in serv_addr;
  char input[16] = { 0 };
  char message[64] = { 0 };
  char buffer[1024] = { 0 };
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }
  printf("Stdscr:%d\n",stdscr);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary
  // form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
    <= 0) {
    printf(
        "\nInvalid address/ Address not supported \n");
    return -1;
  }

  if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
  {
    printf("\nConnection Failed \n");
    endwin();
    return -1;
  }
  printf("Rory's Client. Connected to server on port %d.\n", PORT);
  int running = 1;
  char got;
  while(running) {
    got = getch();
    sprintf(message,"Rory's Client,%c",got);
    send(client_fd, message, strlen(message), 0);
    memset(message,'\0',sizeof(message));

    valread = read(client_fd, buffer, 1024);
    char valread_str[12];
    int a = 1;
    // if error do something!
  }

  close(client_fd);
  endwin();
  return 0;
}
