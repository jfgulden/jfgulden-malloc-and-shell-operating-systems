#include "non-canonical.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "defs.h"

#define CHAR_NL '\n'
#define CHAR_EOF '\004'
#define CHAR_BACK '\b'
#define CHAR_DEL 127
#define CHAR_ESC '\033'

void delete_char(void);
void move_left(int* line_pos, int n_movs);
void move_right(int* line_pos, int n_movs);
void write_command_from_history(FILE* histfile, int* line_pos);

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
char buffer[BUFLEN];

/* This bit inspired by:
 *
https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
 */
void reset_input_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(void) {
    struct termios tattr;

    /* Make sure stdin is a terminal. */
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr(STDIN_FILENO, &saved_attributes);

    /* Set the funny terminal modes. */
    tcgetattr(STDIN_FILENO, &tattr);
    /* Clear ICANON and ECHO. We'll do a manual echo! */
    tattr.c_lflag &= ~(ICANON | ECHO);
    /* Read one char at a time */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void delete_char() {
    // genera una secuencia de bytes
    // que indican que se debe borrar un byte
    write(STDOUT_FILENO, "\b \b", 3);
}

void delete_line(int* line_pos) {
    for (int i = 0; i < *line_pos; i++) {
        delete_char();
    }

    *line_pos = 0;
    memset(buffer, 0, BUFLEN);
}

void move_left(int* line_pos, int n_movs) {
    if (n_movs > *line_pos) {
        n_movs = *line_pos;
    }

    for (int i = 0; i < n_movs; i++) {
        write(STDOUT_FILENO, "\b", 1);
    }

    *line_pos -= n_movs;

    // move pointer left
}

void move_right(int* line_pos, int n_movs) {
    if (n_movs > (int)strlen(buffer) - *line_pos) {
        n_movs = strlen(buffer) - *line_pos;
    }

    for (int i = 0; i < n_movs; i++) {
        write(STDOUT_FILENO, "\033[C", 3);
    }

    *line_pos += n_movs;
}

char* non_canonical_read_line(char* prompt) {
    char c;
    int line_pos = 0;
    int history_pos = 0;

    char formatted_prompt[ARGSIZE];
    snprintf(formatted_prompt, sizeof formatted_prompt, "%s %s %s\n", COLOR_RED,
             prompt, COLOR_RESET);

#ifndef SHELL_NO_INTERACTIVE
    write(STDOUT_FILENO, formatted_prompt, strlen(formatted_prompt));
    write(STDOUT_FILENO, "$ ", 2);
#endif

    memset(buffer, 0, BUFLEN);

    while (true) {
        read(STDIN_FILENO, &c, 1);

        if (c == CHAR_NL) {
            buffer[line_pos] = END_STRING;
            write(STDOUT_FILENO, &c, 1);

            return buffer;
        }

        if (c == CHAR_EOF) {
            // teclas "Ctrl-D"
            return NULL;
        }

        if (c == CHAR_DEL) {
            // tecla "Backspace"
            if (line_pos == 0) {
                // estamos al comienzo de la pantalla
                continue;
            }

            delete_char();
            buffer[line_pos--] = '\0';
        }

        if (c == CHAR_ESC) {
            // comienzo de una sequencia
            // de escape
            char esc_seq;
            assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

            if (esc_seq != '[') continue;

            assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
            // write(STDOUT_FILENO, esc_seq, 1);

            if (esc_seq == 'H') {
                move_left(&line_pos, BUFLEN);
            }
            if (esc_seq == 'F') {
                move_right(&line_pos, BUFLEN);
            }

            // tecla 'Options+Left' en OSX o 'Ctrl+Left' en Linux

            if (esc_seq == 'A') {
                history_pos++;

                FILE* histfile = get_histfile();
                if (histfile == NULL) {
                    continue;
                }

                delete_line(&line_pos);
                last_n_lines(histfile, history_pos);
                write_command_from_history(histfile, &line_pos);
            }
            if (esc_seq == 'B') {
                history_pos--;

                if (history_pos <= 0) {
                    history_pos = 0;
                    continue;
                }

                FILE* histfile = get_histfile();
                if (histfile == NULL) {
                    continue;
                }

                delete_line(&line_pos);
                last_n_lines(histfile, history_pos);
                write_command_from_history(histfile, &line_pos);
            }
            if (esc_seq == 'C') {
                move_right(&line_pos, 1);
            }
            if (esc_seq == 'D') {
                move_left(&line_pos, 1);
            }
        }

        if (isprint(c)) {  // si es visible
            write(STDOUT_FILENO, &c, 1);
            buffer[line_pos++] = c;
        }
    }
}

void write_command_from_history(FILE* histfile, int* line_pos) {
    char character = fgetc(histfile);
    while (character != '\n') {
        write(STDOUT_FILENO, &character, 1);
        buffer[*line_pos] = character;
        (*line_pos)++;
        character = fgetc(histfile);
    }
}
