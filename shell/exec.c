#include "exec.h"

#include "parsing.h"
#include "stdlib.h"
#include "utils.h"

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
    // Your code here
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
            execvp(e->argv[0], e->argv);
            //perror("fallo al ejecutar el comando");
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
            // Your code here
            r = (struct execcmd *)cmd;

            if (strlen(r->out_file) > 0) {
                int fd = open_redir_fd(r->out_file, O_WRONLY | O_CREAT | O_TRUNC);
                dup2(fd, STDOUT_FILENO);
            }

            if (strlen(r->in_file) > 0) {
                int fd = open_redir_fd(r->in_file, O_RDONLY);
                dup2(fd, STDIN_FILENO);
            }

            if (strlen(r->err_file) > 0) {
                if (strcmp(r->err_file, "&1") == 0) {
                    dup2(STDOUT_FILENO, STDERR_FILENO);
                } else {
                    int fd = open_redir_fd(r->err_file, O_WRONLY | O_CREAT | O_TRUNC);
                    dup2(fd, STDERR_FILENO);
                }
            }

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

            int izq = fork();
            if (izq < 0) {
                perror("error en el fork");
            }

            if (izq == 0) {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[0]);
                close(fds[1]);

                exec_cmd(p->leftcmd);
            }

            int der = fork();
            if (der < 0) {
                perror("error en el fork");
            }

            if (der == 0) {
                dup2(fds[0], STDIN_FILENO);
                close(fds[0]);
                close(fds[1]);

                p->rightcmd = parse_line(p->rightcmd->scmd);
                exec_cmd(p->rightcmd);
            }

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
