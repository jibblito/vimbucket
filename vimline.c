#include "vimline.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct VimLine *initEmptyVimLine()
{
  struct VimLine* emptyVimLine =
    (struct VimLine*)malloc(sizeof(struct VimLine));
  emptyVimLine->end_index=-1;
  sprintf(emptyVimLine->content, ""); 
  return emptyVimLine;
}

struct VimLine *newVimLine(char *str)
{
  struct VimLine* newVimLine =
    (struct VimLine*)malloc(sizeof(struct VimLine));
  setVimLine(newVimLine,str);
  return newVimLine;
}

int setVimLine(struct VimLine *line, char *str)
{
  sprintf(line->content, str); 
  int len = strlen(str);
  if(str[len-1]=='\n')
  {
    len = len-1; // newline not account for len
    line->content[len] = '\0';
  }
  if (len >= MAX_LINELENGTH_DEFAULT) len = MAX_LINELENGTH_DEFAULT; // trim long lines
  else if (len == 0) line->end_index = 0;
  else if (len == -1) line->end_index = -1; // empty line: end_index == -1
  else line->end_index = len-1;
  return 0;
}

void delVimLine(struct VimLine *target)
{
  free(target);
}

char* getVimLine(struct VimLine *target)
{
  return target->content;
}

void addChar(struct VimLine *target, int index, char ch)
{
  if (index > target->end_index)
  {
    int i;
    for(i = target->end_index;i < index && i < MAX_LINELENGTH_DEFAULT; i++)
    {
      target->content[i] = ' ';
    }
    target->end_index = i;
  }
  target->content[index] = ch;
}

int getEndIndex(struct VimLine *target)
{
  return target->end_index;
}
