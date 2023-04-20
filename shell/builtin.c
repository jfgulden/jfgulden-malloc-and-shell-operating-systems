#include "builtin.h"

#include "utils.h"

extern char prompt[PRMTLEN];

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int exit_shell(char *cmd) {
    if (strcmp(cmd, "exit")) return 0;

    return 1;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int cd(char *cmd) {
    if (strncmp(cmd, "cd", 2)) {
        return 0;
    }

    char *dir;

    if (cmd[2] == '\0') {
        dir = getenv("HOME");
    } else if (cmd[2] == ' ') {
        dir = cmd + 3;
    } else {
        return 0;
    }

    if (chdir(dir)) {
        perror("Error al cambiar de directorio");
    } else {
        snprintf(prompt, sizeof prompt, "(%s)", getcwd(NULL, 0));
    }

    return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int pwd(char *cmd) {
    if (strcmp(cmd, "pwd")) {
        return 0;
    }
    printf("%s\n", getcwd(NULL, 0));
    return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int history(char *cmd) {
    if (strncmp(cmd, "history", 7)) {
        return 0;
    }

    FILE *histfile;
    char *histfile_dir = getenv("HISTFILE");
    if (histfile_dir == NULL) {
        histfile_dir = ".fisop_history";
    }

    histfile = fopen(histfile_dir, "r");

    if (cmd[7] == '\0') {
        char line[ARGSIZE];
        while (fgets(line, ARGSIZE, histfile) != NULL) {
            printf("%s", line);
        }

        return 1;
    }

    if (cmd[7] != ' ') {
        return 0;
    }

    int n = atoi(cmd + 8);

    char line[ARGSIZE];
    fseek(histfile, 0, SEEK_END);
    int pos = ftell(histfile);

    int i = 0;
    while (pos && i <= n) {
        fseek(histfile, --pos, SEEK_SET);
        if (fgetc(histfile) == '\n') i++;
    }

    if (pos == 0) {
        fseek(histfile, 0, SEEK_SET);
    }

    while (fgets(line, ARGSIZE, histfile)) {
        fprintf(stdout, "%s", line);
    }

    return 1;
}
