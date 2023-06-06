/*
 * Methods for drawing a Vim window
 */
#include <curses.h>
#include "screen.h"

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

void draw_text_window(WINDOW *window, struct VimLine** file, int scroll_head, int total_lines, int window_height)
{
  int i;
  werase(window);
  for (i = 0; i < total_lines - scroll_head && i < window_height-3; i++)
  {
    wmove(window,i,0);
    waddstr(window,file[i+scroll_head]->content);
  }
  wrefresh(window);
}

