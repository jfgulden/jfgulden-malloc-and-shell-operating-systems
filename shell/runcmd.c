#include "runcmd.h"

int status = 0;
struct cmd *parsed_pipe;

void write_command_history(char *cmd);

// runs the command in 'cmd'
int run_cmd(char *cmd) {
    pid_t p;
    struct cmd *parsed;

    write_command_history(cmd);

    // if the "enter" key is pressed
    // just print the prompt again
    if (cmd[0] == END_STRING) return 0;

    // "history" built-in call
    if (history(cmd)) return 0;

    // "cd" built-in call
    if (cd(cmd)) return 0;

    // "exit" built-in call
    if (exit_shell(cmd)) return EXIT_SHELL;

    // "pwd" built-in call
    if (pwd(cmd)) return 0;

    // parses the command line
    parsed = parse_line(cmd);

       // forks and run the command
    if ((p = fork()) == 0) {
        // keep a reference
        // to the parsed pipe cmd
        // so it can be freed later
        if (parsed->type == PIPE) parsed_pipe = parsed;

        exec_cmd(parsed);
    }

    // stores the pid of the process
    parsed->pid = p;

    waitpid(-1, &status, WNOHANG);

    if (parsed->type == BACK) {
        print_back_info(parsed);
    } else {
        waitpid(p, &status, 0);
        print_status_info(parsed);
    }

    free_command(parsed);

    return 0;
}

void write_command_history(char *cmd) {
    char *histfile_dir = getenv("HISTFILE");
    if (histfile_dir == NULL) {
        histfile_dir = ".fisop_history";
    }

    int histfile =
        open(histfile_dir, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
    if (histfile < 0) {
        perror("error opening history file");
        exit(-1);
    }
    write(histfile, cmd, strlen(cmd));
    write(histfile, "\n", 1);
}