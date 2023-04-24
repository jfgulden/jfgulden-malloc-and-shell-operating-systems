#ifndef NON_CANONICAL_H
#define NON_CANONICAL_H

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void reset_input_mode(void);
void set_input_mode(void);
char *non_canonical_read_line(char *prompt);

#endif  // NON_CANONICAL_H
