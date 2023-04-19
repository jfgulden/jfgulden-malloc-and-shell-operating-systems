#include "builtin.h"
#include "utils.h"

extern char prompt[PRMTLEN];

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0) {
		return 1;
	}
	return 0;
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
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2)) {
		return 0;
	}
	if (cmd[2] == '\0') {
		if (chdir(getenv("HOME"))) {
			perror("Error al cambiar de directorio");
		} else {
			snprintf(prompt, sizeof prompt, "(%s)", getenv("HOME"));
		}
		return 1;
	}
	if (cmd[2] == ' ') {
		char *dir = cmd + 3;
		if (chdir(dir)) {
			perror("Error al cambiar de directorio");
		} else {
			snprintf(prompt, sizeof prompt, "(%s)", getcwd(NULL, 0));
		}
		return 1;
	}
	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0) {
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
int
history(char *cmd)
{
	// Your code here

	return 0;
}
