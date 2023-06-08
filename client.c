// Client side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include "screen.h"
#include "vimline.h"
#define PORT 25565
#define MAX_LINES_DEFAULT 1000

struct VimLine** file;
int i;
char status_msg[128] = { 0 };

void init_file()
{
  file = malloc(sizeof(struct VimLine) * MAX_LINES_DEFAULT);
}


int main(int argc, char const* argv[])
{
  initscr();
  cbreak();
  int status, valread, client_fd;
  struct sockaddr_in serv_addr;
  char input[16] = { 0 };
  char message[64] = { 0 };
  char network_buffer[1024] = { 0 };

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }
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
  int running = 1;
  char got;

  valread = read(client_fd, network_buffer, sizeof(network_buffer));
  snprintf(status_msg, sizeof(status_msg),"Reading file... server says: %s",network_buffer);
  update_output_window(stdscr,status_msg);
  refresh();

  init_file();

  struct VimLine *lineBuffer = initEmptyVimLine();
  i = 0;
  while (read(client_fd, lineBuffer, sizeof(struct VimLine)) != 0)
  {
    if (lineBuffer->end_index != -1)
    {
      file[i] = newVimLine(lineBuffer->content);
      i++;
    } else break;
  }
  free(lineBuffer);
  sprintf(status_msg,"Done reading file... %d lines!", i);
  update_output_window(stdscr,status_msg);
  refresh();

  while(running) {
    got = getch();
    sprintf(message,"Rory's Client,%c",got);
    send(client_fd, message, strlen(message), 0);
    memset(message,'\0',sizeof(message));

    valread = read(client_fd, network_buffer, sizeof(network_buffer));
    sprintf(status_msg,"Server says: %s", network_buffer);
    update_output_window(stdscr,status_msg);
    // if error do something!
  }

  close(client_fd);
  endwin();
  return 0;
}
