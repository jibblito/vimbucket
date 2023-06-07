// Libraries, Dude!
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <curses.h>
#include <fcntl.h>
#include <signal.h>
#include "vimline.h"
#include "screen.h"

#define TRUE 1
#define FALSE 0
#define PORT 25565
#define MAX_LINES_DEFAULT 1000

int server_running = 1;
int master_socket, logfile;
FILE *outputfile;
char output_buffer[64]; // for output messages
int cur_y,cur_x, scroll_head = 0;
int i;
int cur_lines = 1;
WINDOW *text_window,*file_window,*output_window,*numbers_window;
struct VimLine** file;
char* output_filename;
size_t len;

void init_file()
{
  file = malloc(sizeof(struct VimLine) * MAX_LINES_DEFAULT);
}

void write_file()
{
  ftruncate(fileno(outputfile),2);
  rewind(outputfile);
  for (i = 0; i < cur_lines; ++i)
  {
    if (file[i]->end_index!=-1)
    {
      fprintf(outputfile,"%s\n",getVimLine(file[i]));
      dprintf(logfile,"Writing line %d (content: `%s`)!\n",i,getVimLine(file[i]));
    }
    free(file[i]);
  }
  free(file);
  fclose(outputfile);
}

void read_file_into_buffer()
{
  if (access(output_filename, F_OK) == 0) { // current file exists
    outputfile = fopen(output_filename, "r+");
    char* line;
    i = 0;
    while (getline(&line,&len,outputfile) != -1) // load file into buffer
    {
      file[i] = newVimLine(line);
      i++;
    }
    cur_lines = i;
  } else {
    // file doesn't exist, create it
    outputfile = fopen(output_filename, "w+");
    file[0] = newVimLine("New file. Edit it!\n");
  }
}

void sig_handler(int signo)
{
  if (signo == SIGINT)
    printf("[%d] received SIGINT\n", getpid());
    server_running = 0;
    endwin();
    write_file();
    fprintf(stdout,"Server terminated by SIGINT\n");
    dprintf(logfile,"Server terminated by SIGINT\n");
    close(logfile);
    exit(0);
}

void register_signal_handler()
{
  if (signal(SIGINT, sig_handler) == SIG_ERR)
	  printf("\ncan't catch SIGINT\n");
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stdout,"Please provide an output file\n");
    exit(0);
  }
  output_filename = argv[1];

  int opt = TRUE;
  int new_socket, client_socket[30],
    max_clients = 30, activity, i, valread, sd;
  int max_sd;
  char buffer[64] = { 0 };
  char vim_mode = 0;
  len = COLS;
  int lines_expand_threshold = 0.9 * (float) MAX_LINES_DEFAULT;

  initscr();
  initialize_colors(); // screen.c

  init_file();
  read_file_into_buffer();
  logfile = open("log.txt", O_CREAT | O_TRUNC | O_RDWR, S_IWUSR | S_IRUSR);

  text_window = newwin(LINES-2,COLS-4,0,4);
  file_window = newwin(1,COLS,LINES-2,0);
  output_window = newwin(1,COLS,LINES-1,0);
  numbers_window = newwin(LINES-2,4,0,0);
//  wattrset(file_window,COLOR_PAIR(black_white)); // ARCHAIC - move to screen.c

  register_signal_handler();
  cbreak();

  socklen_t peer_addr_size;
  struct sockaddr_in address;

  fd_set readfds;

  char *message = "Welcome to Vimbucket, v1.0";

  // initialise all client_socket[] to 0
  for (i = 0; i < max_clients; i++)  
  {  
      client_socket[i] = 0;  
  }

  // Creating socket file descriptor
  if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port
  if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
        sizeof(opt)) < 0)
  {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  
  // type of socket created: in address, of course
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Bind socket to localhost
  if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) 
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(master_socket, 3) < 0)
  {
      perror("listen");
      exit(EXIT_FAILURE);
  }

  sprintf(output_buffer,"vimbucket server. press ctrl-c to stop it running...", PORT);

  update_output_window(output_window,output_buffer);
  draw_numbers_window(numbers_window, scroll_head, cur_lines, LINES-3);
  draw_statusbar(file_window,output_filename, cur_y, cur_x, cur_lines);
  draw_text_window(text_window, file, scroll_head, cur_lines, LINES-3);
  wmove(text_window,0,0);

  wrefresh(output_window);
  wrefresh(file_window);
  wrefresh(numbers_window);
  wrefresh(text_window);
  int addrlen = sizeof(address);

  for (;;)
  {
    FD_ZERO(&readfds);
    FD_SET(master_socket, &readfds); // add master socket to set 
    max_sd = master_socket;

    // check all child sockets for valid reads
    for (i = 0; i < max_clients; i++)
    {
      // socket descriptor
      sd = client_socket[i];

      // if valid file descriptor, add to read list
      if (sd > 0)
        FD_SET(sd, &readfds);

      // highest file descriptor, need it for select()
      if (sd > max_sd)
        max_sd = sd;
    }

    // Only read fds are valid, timeout is null (man select.2)
    // will wait indefinitely, select() will
    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL); 

    if (activity < 0 && (errno!=EINTR))
    {
      printf("select error\n");
    }

    if (FD_ISSET(master_socket, &readfds))
    {
      if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
      {
          perror("accept");
          exit(EXIT_FAILURE);
      }
      //inform user of socket number - used in send and receive commands 
      dprintf(logfile,"New connection , socket fd is %d , ip is : %s , port : %d\n",
          new_socket, inet_ntoa(address.sin_addr), ntohs
          (address.sin_port));  


      if(send(new_socket, message, strlen(message), 0) != strlen(message) )  
      {  
          perror("send");  
      }

      // add new socket to array of clients
      for (i=0; i < max_clients; i++)
      {
        if (client_socket[i]==0)
        {
          client_socket[i] = new_socket;
          dprintf(logfile,"Adding to list of sockets as number %d!\n",i);
          sprintf(output_buffer,"Client %d has joined!\0nig",i);
          update_output_window(output_window,output_buffer);
          wmove(text_window,cur_y,cur_x);
          wrefresh(output_window);
          wrefresh(text_window);
          break;
        }
      }
    }
    // check all slave sockets for char input
    for (i = 0; i < max_clients; i++)
    {
      sd = client_socket[i];

      if (FD_ISSET(sd, &readfds))
      {
        //Check if it was for closing , and also read the 
        //incoming message 
        if ((valread = read(sd , buffer, sizeof(buffer))) == 0)  
        {  
            //Somebody disconnected , get his details and print 
            getpeername(sd , (struct sockaddr*)&address , \
                (socklen_t*)&addrlen);
            dprintf(logfile,"Host disconnected , ip %s , port %d \n" ,
                  inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                 
            //Close the socket and mark as 0 in list for reuse 
            close( sd );  
            client_socket[i] = 0;  
        } else {  
            if (valread == -1)
            { 
              sprintf(output_buffer,"Client %d has left: %s\0",i,strerror(errno));
              update_output_window(output_window,output_buffer);
              wmove(text_window,0,0);
              continue;
            }
            // Extract tokens from message
            char* clientname = strtok(buffer,",");
            char* message = strtok(NULL ,",");
            char input;
            if (message == NULL) { // account for commas - packet is a CSV
              input = ',';
            } else {
              input = message[0];
            }
            if (vim_mode == 0) {
              switch(input)
              {
                case 'h':
                  if (cur_x!=0) cur_x--;
                  sprintf(output_buffer,"X:%d!",cur_x);
                  update_output_window(output_window,output_buffer);
                  break;
                case 'j':
                  if (cur_y < cur_lines-1)
                  {
                    if (cur_y == LINES-3)
                    {
                      scroll_head+=1;
                    }
                    cur_y++;
                    sprintf(output_buffer,"Y:%d!",cur_y);
                    update_output_window(output_window,output_buffer);
                  }
                  break;
                case 'k':
                  if (cur_y >= 0)
                  {
                    if (cur_y == 0)
                    {
                      if (scroll_head != 0) scroll_head-=1;
                    } else cur_y--;
                    sprintf(output_buffer,"Y:%d!",cur_y);
                    update_output_window(output_window,output_buffer);
                  }
                  break;
                case 'l':
                  if (cur_x!=COLS-1) cur_x++;
                  sprintf(output_buffer,"X:%d!",cur_x);
                  update_output_window(output_window,output_buffer);
                  break;
                case 'i':
                  vim_mode = 1;
                  sprintf(output_buffer,"Switched to Insert Mode!");
                  update_output_window(output_window,output_buffer);
                  break;
                case 'a':
                  vim_mode = 1;
                  cur_x++;
                  sprintf(output_buffer,"Switched to Insert Mode!");
                  update_output_window(output_window,output_buffer);
                  break;
                case '$':
                  cur_x = getEndIndex(file[cur_y]);
                  sprintf(output_buffer,"Moved To End Of Line %d!",cur_y);
                  update_output_window(output_window,output_buffer);
                  break;
                case '0':
                  cur_x = 0;
                  sprintf(output_buffer,"Moved To Start Of Line %d!",cur_y);
                  update_output_window(output_window,output_buffer);
                  break;
                case 5:
                  if (scroll_head != cur_lines-1){
                    scroll_head+=1;
                  }
                  sprintf(output_buffer,"Window down!");
                  update_output_window(output_window,output_buffer);
                  break;
                case 25:
                  if (scroll_head !=0) {
                    scroll_head-=1;
                  }
                  sprintf(output_buffer,"Window up!");
                  update_output_window(output_window,output_buffer);
                  break;
              }
            } else if (vim_mode == 1) {
              if (input == 27) // escape
              {
                vim_mode = 0;
                sprintf(output_buffer,"Switched to Move Mode!");
                update_output_window(output_window,output_buffer);
              } else {
                waddch(text_window,input);
                getyx(text_window,cur_y,cur_x);
                if (input == 10) // enter
                {
                  if (cur_lines == cur_y)
                  {
                    file[cur_y + scroll_head] = newVimLine("\n");
                  } else {
                    file[cur_lines] = newVimLine(file[cur_y]->content);
                    setVimLine(file[cur_y + scroll_head],"\n");
                  }
                  cur_lines++;
                } else addChar(file[cur_y + scroll_head],cur_x-1,input);
                sprintf(output_buffer,"Char written: %d!", input);
                update_output_window(output_window,output_buffer);
              }
            }

            draw_text_window(text_window, file, scroll_head, cur_lines, LINES-3);
            draw_statusbar(file_window, output_filename, cur_y, cur_x, cur_lines);
            draw_numbers_window(numbers_window, scroll_head, cur_lines, LINES-3);
            wmove(text_window,cur_y,cur_x);
            wrefresh(output_window);
            wrefresh(file_window);
            wrefresh(numbers_window);
            wrefresh(text_window);

            char response[64] = { 0 };
            sprintf(response,"vimbucket accepted your input: %s",message);
            send(sd, response, 64, 0);
            memset(buffer,'\0',sizeof(buffer));
        }

      }
    }
  }
 
  return 0;

}
