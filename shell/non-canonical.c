#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "defs.h"

#define CHAR_NL '\n'
#define CHAR_EOF '\004'
#define CHAR_BACK '\b'
#define CHAR_DEL 127
#define CHAR_ESC '\033'

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

char* non_canonical_read_line(char* prompt) {
    char c;
    int line_pos = 0;

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
            if (esc_seq == 'A') {
            }
            if (esc_seq == 'B') {
                        }
            if (esc_seq == 'C') {
                if (line_pos == strlen(buffer)) {
                    // estamos al final de la pantalla
                    continue;
                }
                write(STDOUT_FILENO, "\033[C", 3);
                line_pos++;
            }
            if (esc_seq == 'D') {
                // // flecha "izquierda"
                // return "flecha izquierda\n";
                if (line_pos == 0) {
                    // estamos al comienzo de la pantalla
                    continue;
                }

                write(STDOUT_FILENO, "\b", 1);
                line_pos--;
            }
        }

        if (isprint(c)) {  // si es visible
            assert(write(STDOUT_FILENO, &c, 1) > 0);
            buffer[line_pos++] = c;
        }
    }
}

void move_left() {
    // move pointer left
    write(STDOUT_FILENO, "\b", 1);
}

void move_right() {
    // move pointer right
}