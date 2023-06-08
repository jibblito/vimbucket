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
WINDOW *text_window,*file_window,*output_window,*numbers_window;

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

  text_window = newwin(LINES-2,COLS-4,0,4);
  file_window = newwin(1,COLS,LINES-2,0);
  output_window = newwin(1,COLS,LINES-1,0);
  numbers_window = newwin(LINES-2,4,0,0);


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
  update_output_window(output_window,status_msg);
  wrefresh(output_window);

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
  int cur_lines = i;
  free(lineBuffer);
  sprintf(status_msg,"Done reading file... %d lines!", i);

  int cur_y = 0;
  int cur_x = 0;
  int scroll_head = 0;
  update_output_window(output_window,status_msg);
  draw_numbers_window(numbers_window, scroll_head, cur_lines, LINES-3);
  draw_statusbar(file_window,"output filename", cur_y, cur_x, cur_lines);
  draw_text_window(text_window, file, scroll_head, cur_lines, LINES-3);
  wmove(text_window,0,0);

  wrefresh(output_window);
  wrefresh(file_window);
  wrefresh(numbers_window);
  wrefresh(text_window);


  while(running) {
    got = wgetch(text_window);
    sprintf(message,"Rory's Client,%c",got);
    send(client_fd, message, strlen(message), 0);
    memset(message,'\0',sizeof(message));

    valread = read(client_fd, network_buffer, sizeof(network_buffer));
    sprintf(status_msg,"Server says: %s", network_buffer);

    update_output_window(output_window,status_msg);
    draw_numbers_window(numbers_window, scroll_head, cur_lines, LINES-3);
    draw_statusbar(file_window,"output filename", cur_y, cur_x, cur_lines);
    draw_text_window(text_window, file, scroll_head, cur_lines, LINES-3);
    wmove(text_window,0,0);

    wrefresh(output_window);
    wrefresh(file_window);
    wrefresh(numbers_window);
    wrefresh(text_window);
    // if error do something!
  }

  close(client_fd);
  endwin();
  return 0;
}
