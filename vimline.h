#ifndef VIMLINE_H
#define VIMLINE_H

#define MAX_LINELENGTH_DEFAULT 200

struct VimLine {
  char line[MAX_LINELENGTH_DEFAULT];
  int end_index;
};

struct VimLine* newVimLine(char *str);
int setVimLine(struct VimLine *line, char *str);
char* getVimLine(struct VimLine *line);

void delVimLine(struct VimLine *target);
void addChar(struct VimLine *target, int index, char ch);

int getEndIndex(struct VimLine *target);

#endif
