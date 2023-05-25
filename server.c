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
#define TRUE 1
#define FALSE 0
#define PORT 25565

int server_running = 1;
int master_socket, logfile;
FILE *outputfile;
char output_buffer[64]; // for output messages
int cur_y,cur_x, i;
WINDOW *text_window;
WINDOW *file_window;
WINDOW *status_window;
char** file;
char* output_filename;

void init_file()
{
  file = malloc(sizeof(char*) * LINES-2);
  for (i = 0; i < LINES; i++)
  {
    file[i] = malloc(sizeof(char)*COLS);
  }
}

void write_file()
{
  ftruncate(fileno(outputfile),2);
  rewind(outputfile);
  for (i = 0; i < LINES; i++)
  {
    fprintf(outputfile,file[i]);
    dprintf(logfile,"Writing line %d to outputfile!\n",i);
    free(file[i]);
  }
  free(file);
  fclose(outputfile);
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
    scr_dump("./screen.txt");
    close(logfile);
    exit(0);
}

void register_signal_handler()
{
  if (signal(SIGINT, sig_handler) == SIG_ERR)
	  printf("\ncan't catch SIGINT\n");
}


void write_buffer_to_output()
{
  wmove(status_window,0,0);
  wclrtoeol(status_window);
  waddstr(status_window,output_buffer);
  wmove(text_window,cur_y,cur_x); // current y and x set before calling w_b_t_o()
  wrefresh(status_window);
  wrefresh(text_window);
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
  size_t len = COLS;

  initscr();
  init_file();
  if (access(output_filename, F_OK) == 0) { // read file into buffer
    outputfile = fopen(output_filename, "r+");
    for (i = 0; i < LINES; i++)
    {
      char* line;
      if (getline(&line,&len,outputfile) != -1) sprintf(file[i],line);
      else sprintf(file[i],"\0");
    }
  } else {
    // file doesn't exist, create it
    outputfile = fopen(output_filename, "w+");
    sprintf(file[0],"New file. Edit it!");
  }
  logfile = open("log.txt", O_CREAT | O_TRUNC | O_RDWR, S_IWUSR | S_IRUSR);

  text_window = newwin(LINES-2,COLS,0,0);
  file_window = newwin(1,COLS,LINES-2,0);
  status_window = newwin(1,COLS,LINES-1,0);
  register_signal_handler();
  cbreak();
  scrollok(stdscr,true);
  for (i = 0; i < LINES-2; i++)
  {
    mvwaddstr(text_window,i,0,file[i]);
  }
  waddstr(file_window,"File");
  waddstr(status_window,"Status");
  wmove(text_window,0,0);
  wrefresh(text_window);
  wrefresh(file_window);
  wrefresh(status_window);

  socklen_t peer_addr_size;
  struct sockaddr_in address;

  fd_set readfds;

  char *message = "Welcome to Vomitbucket, v1.0";

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
  sprintf(output_buffer,"vomitbucket server. press ctrl-c to stop it running...", PORT);
  write_buffer_to_output(output_buffer);
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

      dprintf(logfile,"Welcome message sent successfully\n");

      // add new socket to array of clients
      for (i=0; i < max_clients; i++)
      {
        if (client_socket[i]==0)
        {
          client_socket[i] = new_socket;
          dprintf(logfile,"Adding to list of sockets as number %d!\n",i);
          sprintf(output_buffer,"Client %d has joined!\0nig",i);
          write_buffer_to_output();
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
              write_buffer_to_output();
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
                  write_buffer_to_output();
                  break;
                case 'j':
                  if (cur_y == LINES-3)
                  {
                    wscrl(text_window,1);
                  }
                  cur_y++;
                  sprintf(output_buffer,"Y:%d!",cur_y);
                  write_buffer_to_output();
                  break;
                case 'k':
                  if (cur_y == 0) 
                  {
                    wscrl(text_window,-1); 
                  }
                  cur_y--;
                  sprintf(output_buffer,"Y:%d!",cur_y);
                  write_buffer_to_output();
                  break;
                case 'l':
                  if (cur_x!=COLS-1) cur_x++;
                  sprintf(output_buffer,"X:%d!",cur_x);
                  write_buffer_to_output();
                  break;
                case 'i':
                  vim_mode = 1;
                  sprintf(output_buffer,"Switched to Insert Mode!");
                  write_buffer_to_output();
                  break;
                case 'a':
                  vim_mode = 1;
                  cur_x++;
                  sprintf(output_buffer,"Switched to Insert Mode!");
                  write_buffer_to_output();
                  break;
              }
            } else if (vim_mode == 1) {
              if (input == 27)
              {
                vim_mode = 0;
                sprintf(output_buffer,"Switched to Move Mode!");
                write_buffer_to_output();
              } else {
                waddch(text_window,input);
                getyx(text_window,cur_y,cur_x);
                file[cur_y][cur_x-1] = input;
                sprintf(output_buffer,"Char written: %d!", input);
                write_buffer_to_output();
              }
            }
            //addch(input);
            wrefresh(status_window);
            wrefresh(file_window);
            wrefresh(text_window);
            char response[64] = { 0 };
            sprintf(response,"vomitbucket accepted your input: %s",message);
            send(sd, response, 64, 0);
            memset(buffer,'\0',sizeof(buffer));
        }

      }
    }
  }
 
  return 0;

}