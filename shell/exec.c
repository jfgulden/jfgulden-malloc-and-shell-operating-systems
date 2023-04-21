#include "exec.h"

#include "parsing.h"
#include "stdlib.h"
#include "utils.h"

void handle_output_redir(char *out_file);
void handle_input_redir(char *in_file);
void handle_error_redir(char *err_file);
void handle_left_child(struct pipecmd *p, int fds[2]);
void handle_right_child(struct pipecmd *p, int fds[2]);

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void get_environ_key(char *arg, char *key) {
    int i;
    for (i = 0; arg[i] != '='; i++) key[i] = arg[i];

    key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void get_environ_value(char *arg, char *value, int idx) {
    size_t i, j;
    for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++) value[j] = arg[i];

    value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void set_environ_vars(char **eargv, int eargc) {
    for (int i = 0; i < eargc; i++) {
        int position_equal = block_contains(eargv[i], '=');

        if (position_equal < 0) continue;

        char key[ARGSIZE];
        char value[ARGSIZE];

        get_environ_key(eargv[i], key);
        get_environ_value(eargv[i], value, position_equal);

        if (setenv(key, value, 1) == -1) return;
    }
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int open_redir_fd(char *file, int flags) {
    int fd = open(file, flags | O_CLOEXEC, S_IWUSR | S_IRUSR);
    if (fd < 0) {
        perror("error opening file");
        exit(-1);
    }
    return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void exec_cmd(struct cmd *cmd) {
    // To be used in the different cases
    struct execcmd *e;
    struct backcmd *b;
    struct execcmd *r;
    struct pipecmd *p;

    switch (cmd->type) {
        case EXEC:
            e = (struct execcmd *)cmd;
            set_environ_vars(e->eargv, e->eargc);

            execvp(e->argv[0], e->argv);

            perror("fallo al ejecutar el comando");
            exit(-1);
            break;

        case BACK: {
            b = (struct backcmd *)cmd;

            exec_cmd(b->c);
            break;
        }

        case REDIR: {
            // changes the input/output/stderr flow
            //
            // To check if a redirection has to be performed
            // verify if file name's length (in the execcmd struct)
            // is greater than zero
            //
            r = (struct execcmd *)cmd;
            set_environ_vars(r->eargv, r->eargc);

            handle_output_redir(r->out_file);
            handle_input_redir(r->in_file);
            handle_error_redir(r->err_file);

            execvp(r->argv[0], r->argv);

            perror("fallo al ejecutar el comando");
            exit(-1);
            break;
        }

        case PIPE: {
            p = (struct pipecmd *)cmd;

            int fds[2];
            if (pipe(fds) < 0) {
                perror("error creating pipe");
                exit(-1);
            }

            handle_left_child(p, fds);
            handle_right_child(p, fds);

            close(fds[0]);
            close(fds[1]);

            waitpid(-1, NULL, 0);
            waitpid(-1, NULL, 0);

            free_command(parsed_pipe);

            exit(0);

            break;
        }
    }
}

void handle_output_redir(char *out_file) {
    if (strlen(out_file) == 0) return;

    int fd = open_redir_fd(out_file, O_WRONLY | O_CREAT | O_TRUNC);
    dup2(fd, STDOUT_FILENO);
}

void handle_input_redir(char *in_file) {
    if (strlen(in_file) == 0) return;

    int fd = open_redir_fd(in_file, O_RDONLY);
    dup2(fd, STDIN_FILENO);
}

void handle_error_redir(char *err_file) {
    if (strlen(err_file) == 0) return;

    if (strcmp(err_file, "&1") == 0) {
        dup2(STDOUT_FILENO, STDERR_FILENO);
    } else {
        int fd = open_redir_fd(err_file, O_WRONLY | O_CREAT | O_TRUNC);
        dup2(fd, STDERR_FILENO);
    }
}

void handle_left_child(struct pipecmd *p, int fds[2]) {
    int left_fork = fork();
    if (left_fork < 0) {
        perror("error en el fork");
        exit(-1);
    }

    if (left_fork == 0) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);

        exec_cmd(p->leftcmd);
    }
}

void handle_right_child(struct pipecmd *p, int fds[2]) {
    int right_fork = fork();
    if (right_fork < 0) {
        perror("error en el fork");
        exit(-1);
    }

    if (right_fork == 0) {
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);

        p->rightcmd = parse_line(p->rightcmd->scmd);
        exec_cmd(p->rightcmd);
    }
}