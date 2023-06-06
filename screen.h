/*
 * Methods for drawing a Vim window
 */
#include <curses.h>
#include "vimline.h"

void draw_statusbar(WINDOW *window, char *filename, int cur_y, int cur_x,
    int total_lines);

void draw_text_window(WINDOW *window, struct VimLine** file, int scroll_head,
    int total_lines, int window_height);
