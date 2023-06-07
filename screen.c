/*
 * Methods for drawing a Vim window
 */
#include <curses.h>
#include "screen.h"

short black_white=1, white_clear=2, yellow_clear=3, blue_clear=4; //colors

void initialize_colors()
{
  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(black_white, COLOR_BLACK, COLOR_WHITE);
    init_pair(white_clear, COLOR_WHITE, -1);
    init_pair(yellow_clear, COLOR_YELLOW, -1);
    init_pair(blue_clear, COLOR_BLUE, -1);
  }
}

void draw_statusbar(WINDOW *window, char *filename, int cur_y, int cur_x, int total_lines)
{
  // filename ' line,index ' percentage%
  char curloc_s[20], curpercent_s[4];
  sprintf(curloc_s,"%d,%d",cur_y,cur_x);
  sprintf(curpercent_s,"%d%%",(int)(((float)cur_y/(float)total_lines)*100));

  werase(window);
  wmove(window,0,0);
  waddstr(window,filename);
  waddstr(window,"  ");
  waddstr(window,curloc_s);
  waddstr(window,"  ");
  waddstr(window,curpercent_s);
  wrefresh(window); // atomic - refresh inside function
}

void draw_text_window(WINDOW *window, struct VimLine** file, int scroll_head,
    int total_lines, int window_height)
{
  int i;
  werase(window);
  for (i = 0; i < total_lines - scroll_head && i < window_height; i++)
  {
    wmove(window,i,0);
    waddstr(window,file[i+scroll_head]->content);
  }
}

void draw_numbers_window(WINDOW *window, int scroll_head, int total_lines,
    int window_height)
{
  int i;
//  wattron(numbers_window,COLOR_PAIR(yellow_clear));
  werase(window);
  char number[4];
  for (i = 0; i < total_lines - scroll_head && i < window_height; i++)
  {
    sprintf(number,"%d",i+scroll_head+1);
    wmove(window,i,0);
    waddstr(window,number);
  }
//  wattron(numbers_window,COLOR_PAIR(blue_clear));
  for (; i <= window_height; i++)
  {
    wmove(window,i,0);
    waddstr(window,"~");
  }
}

void update_output_window(WINDOW *window, char *message)
{
  wmove(window,0,0);
  wclrtoeol(window);
  waddstr(window,message);
}


