#include "vimline.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct VimLine* newVimLine(char *str)
{
  struct VimLine* newVimLine
    = (struct VimLine*)malloc(sizeof(struct VimLine));
  setVimLine(newVimLine,str);
  return newVimLine;
}

int setVimLine(struct VimLine *line, char *str)
{
  sprintf(line->line, str); 
  int len = strlen(str);
  if (len >= MAX_LINELENGTH_DEFAULT) len = MAX_LINELENGTH_DEFAULT; // trim long lines
  line->end_index = len-1;
  return 0;
}

void delVimLine(struct VimLine *target)
{
  free(target);
}

char* getVimLine(struct VimLine *target)
{
  return target->line;
}

void addChar(struct VimLine *target, int index, char ch)
{
  if (index > target->end_index)
  {
    int i;
    for(i = target->end_index;i < index && i < MAX_LINELENGTH_DEFAULT; i++)
    {
      target->line[i] = ' ';
    }
    target->end_index = i;
  }
  target->line[index] = ch;
}

int getEndIndex(struct VimLine *target)
{
  return target->end_index;
}
