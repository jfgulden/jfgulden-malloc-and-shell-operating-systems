#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "defs.h"
#include "non-canonical.h"
#include "readline.h"
#include "runcmd.h"
#include "types.h"

char prompt[PRMTLEN] = { 0 };

// runs a shell command
static void
run_shell()
{
	char *cmd;

	if (isatty(STDIN_FILENO)) {
		set_input_mode();
		while ((cmd = non_canonical_read_line(prompt)) != NULL) {
			if (run_cmd(cmd) == EXIT_SHELL)
				return;
		}
		reset_input_mode();
	} else {
		while ((cmd = read_line(prompt)) != NULL) {
			if (run_cmd(cmd) == EXIT_SHELL)
				return;
		}
	}

	return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}