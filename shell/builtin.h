#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"

extern char prompt[PRMTLEN];

FILE *get_histfile();
void last_n_lines(FILE *histfile, int n);

int cd(char *cmd);

int exit_shell(char *cmd);

int pwd(char *cmd);

int history(char *cmd);

#endif  // BUILTIN_H
