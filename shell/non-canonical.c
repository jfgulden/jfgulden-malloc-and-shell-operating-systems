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
int move_left(int n_movs);
int move_right(int n_movs);
void write_command_from_history(FILE* histfile);
void render_line(int new_pos);
void delete_line();

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;

char buffer[BUFLEN];

int line_pos = 0;

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

void delete_line() {
    int line_pos_aux = move_right(BUFLEN);
    for (size_t i = 0; i < strlen(buffer); i++) {
        delete_char();
    }
}

int move_left(int n_movs) {
    if (n_movs > line_pos) {
        n_movs = line_pos;
    }

    for (int i = 0; i < n_movs; i++) {
        write(STDOUT_FILENO, "\b", 1);
    }

    return line_pos - n_movs;
}

int move_right(int n_movs) {
    if (n_movs > (int)strlen(buffer) - line_pos) {
        n_movs = strlen(buffer) - line_pos;
    }

    for (int i = 0; i < n_movs; i++) {
        write(STDOUT_FILENO, "\033[C", 3);
    }

    return line_pos + n_movs;
}

char* non_canonical_read_line(char* prompt) {
    char c;
    line_pos = 0;
    int history_pos = 0;

    memset(buffer, 0, BUFLEN);

    char formatted_prompt[BUFLEN];
    snprintf(formatted_prompt, sizeof formatted_prompt, "%s %s %s\n", COLOR_RED,
             prompt, COLOR_RESET);

#ifndef SHELL_NO_INTERACTIVE
    write(STDOUT_FILENO, formatted_prompt, strlen(formatted_prompt));
    write(STDOUT_FILENO, "$ ", 2);
#endif

    while (true) {
        read(STDIN_FILENO, &c, 1);

        // tecla "Enter"
        if (c == CHAR_NL) {
            buffer[line_pos] = END_STRING;
            write(STDOUT_FILENO, &c, 1);

            return buffer;
        }

        // teclas "Ctrl-D"
        if (c == CHAR_EOF) {
            return NULL;
        }

        // tecla "Backspace"
        if (c == CHAR_DEL) {
            if (line_pos == 0) {
                continue;
            }

            delete_line();

            for (int i = line_pos; i < (int)strlen(buffer); i++) {
                buffer[i - 1] = buffer[i];
            }
            buffer[strlen(buffer) - 1] = '\0';

            render_line(line_pos - 1);
        }

        if (c == CHAR_ESC) {
            char esc_seq;
            read(STDIN_FILENO, &esc_seq, 1);

            if (esc_seq != '[') continue;

            read(STDIN_FILENO, &esc_seq, 1);

            // tecla "Home"
            if (esc_seq == 'H') {
                line_pos = move_left(BUFLEN);
            }
            // tecla "End"
            if (esc_seq == 'F') {
                line_pos = move_right(BUFLEN);
            }

            if (esc_seq == '1') {
                read(STDIN_FILENO, &esc_seq, 1);
                read(STDIN_FILENO, &esc_seq, 1);
                read(STDIN_FILENO, &esc_seq, 1);
                // tecla "Ctrl + flecha izquierda"
                if (esc_seq == 'D') {
                    while (line_pos > 0 && buffer[line_pos - 2] != ' ') {
                        line_pos = move_left(1);
                    }
                }
                // tecla "Ctrl + flecha derecha"
                if (esc_seq == 'C') {
                    while (line_pos < (int)strlen(buffer) &&
                           buffer[line_pos + 1] != ' ') {
                        line_pos = move_right(1);
                    }
                }
            }

            // tecla "flecha arriba"
            if (esc_seq == 'A') {
                history_pos++;

                FILE* histfile = get_histfile();
                if (histfile == NULL) {
                    continue;
                }

                delete_line();
                line_pos = 0;
                memset(buffer, 0, BUFLEN);
                last_n_lines(histfile, history_pos);
                write_command_from_history(histfile);
            }
            // tecla "flecha abajo"
            if (esc_seq == 'B') {
                history_pos--;

                if (history_pos <= 0) {
                    delete_line();
                    line_pos = 0;
                    memset(buffer, 0, BUFLEN);

                    history_pos = 0;
                    continue;
                }

                FILE* histfile = get_histfile();
                if (histfile == NULL) {
                    continue;
                }

                delete_line();
                line_pos = 0;
                memset(buffer, 0, BUFLEN);
                last_n_lines(histfile, history_pos);
                write_command_from_history(histfile);
            }
            // tecla "flecha derecha"
            if (esc_seq == 'C') {
                line_pos = move_right(1);
            }
            // tecla "flecha izquierda"
            if (esc_seq == 'D') {
                line_pos = move_left(1);
            }
        }

        if (isprint(c)) {  // si es visible
            delete_line();

            for (int i = strlen(buffer); i > line_pos; i--) {
                buffer[i] = buffer[i - 1];
            }
            buffer[line_pos] = c;

            render_line(line_pos + 1);
        }
    }
}

void write_command_from_history(FILE* histfile) {
    char character = fgetc(histfile);
    while (character != '\n') {
        write(STDOUT_FILENO, &character, 1);
        buffer[line_pos] = character;
        line_pos++;
        character = fgetc(histfile);
    }
}

void render_line(int new_pos) {
    for (size_t i = 0; i < strlen(buffer); i++) {
        write(STDOUT_FILENO, &buffer[i], 1);
    }

    line_pos = strlen(buffer);

    line_pos = move_left(line_pos - new_pos);
}
