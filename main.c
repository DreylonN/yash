#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <signal.h>
#include "jobs.h"

int parseInput(const char *input, char args[][30], int maxArgs);
int redirectionHandler(char *args[], int *in_fd, int *out_fd, int *error_fd);
void pipeHandler(char *args[]);
int isValidCommand(char *args[]);
void setup_signal_handlers();

pid_t fg_pgid = 0;  // Track foreground process group

int main() {
    setup_signal_handlers();

    while (1) {
        char *prompt = readline("# ");
        if (prompt == NULL) {
            exit(0); // Exit shell on EOF
        }

        const int maxArgs = 50;
        char args[maxArgs][30];
        int argCount = parseInput(prompt, args, maxArgs);

        if (argCount > 0) {
            char *argv[maxArgs + 1];
            for (int i = 0; i < argCount; i++) {
                argv[i] = args[i];
            }
            argv[argCount] = NULL;

            if (strcmp(argv[0], "fg") == 0) {
                fgCommand();
                free(prompt);
                continue;
            } else if (strcmp(argv[0], "bg") == 0) {
                bgCommand();
                free(prompt);
                continue;
            } else if (strcmp(argv[0], "jobs") == 0) {
                jobsCommand();
                free(prompt);
                continue;
            }

            fg_pgid = 0;  // Reset foreground process group before each command

            // Check for Invalid Commands
            if (!isValidCommand(argv)) {
                free(prompt);
                continue; // Ignore and print new prompt
            }

            // Check if command contains a pipe
            int containsPipe = 0;
            for (int i = 0; argv[i] != NULL; i++) {
                if (strcmp(argv[i], "|") == 0) {
                    containsPipe = 1;
                    break;
                }
            }

            // Check if last argument is '&' for background execution
            int isBackground = (strcmp(argv[argCount - 1], "&") == 0);

            // Handle unsupported background piping
            if (containsPipe && isBackground) {
                printf("Background piping is not supported.\n");
                free(prompt);
                continue;
            }

            // Handle background execution with file redirection
            if (isBackground) {
                argv[argCount - 1] = NULL; // Remove '&' from arguments
                pid_t pid = fork();
                if (pid == 0) { // Child process
                    setpgid(0, 0); // Create a new process group
                    int in_fd = 0, out_fd = 1, error_fd = 2;
                    redirectionHandler(argv, &in_fd, &out_fd, &error_fd);

                    if (in_fd != 0) dup2(in_fd, STDIN_FILENO);
                    if (out_fd != 1) dup2(out_fd, STDOUT_FILENO);
                    if (error_fd != 2) dup2(error_fd, STDERR_FILENO);

                    execvp(argv[0], argv);
                    perror("Command execution failed");
                    exit(1);
                } else { // Parent process
                    setpgid(pid, pid);
                    addJob(pid, "Running", prompt);
                    printf("[%d] %d\n", jobCount, pid);
                }
                free(prompt);
                continue;
            }

            if (containsPipe) {
                pipeHandler(argv); // Handle piping with file redirection
            } else { // Normal execution
                int in_fd = 0, out_fd = 1, error_fd = 2;
                int redirectErr = redirectionHandler(argv, &in_fd, &out_fd, &error_fd);

                if (redirectErr) {
                    free(prompt);
                    continue; // Ignore and print new prompt
                }

                // DEBUG: Print arguments
                // for (int i = 0; argv[i] != NULL; i++) {
                //     printf("argv[%d]: %s\n", i, argv[i]);
                // }

                pid_t pid = fork();
                if (pid < 0) {
                    perror("Fork failed");
                } else if (pid == 0) {
                    setpgid(0, 0);  // Create a new process group for the command
                    tcsetpgrp(STDIN_FILENO, getpid());  // Give it terminal control
                    
                    if (in_fd != 0) dup2(in_fd, STDIN_FILENO);
                    if (out_fd != 1) dup2(out_fd, STDOUT_FILENO);
                    if (error_fd != 2) dup2(error_fd, STDERR_FILENO);

                    //DEBUG: Prints fds
                    // printf("Input redirected to file descriptor %d\n", in_fd);
                    // printf("Output redirected to file descriptor %d\n", out_fd);
                    // printf("Error redirected to file descriptor %d\n", error_fd);

                    execvp(argv[0], argv);
                    //perror("Commands execution failed");
                    exit(1);
                } else {
                    fg_pgid = pid;  // Track foreground process
                    setpgid(pid, pid);  // Ensure PGID is correctly assigned
                    tcsetpgrp(STDIN_FILENO, pid);  // Set terminal foreground process

                    int status;
                    waitpid(pid, &status, WUNTRACED);  // Track if stopped

                    if (WIFSTOPPED(status)) {
                        addJob(pid, "Stopped", prompt);
                        printf("\n[%d]+  Stopped\t\t%s\n", jobCount, prompt);
                    }

                    tcsetpgrp(STDIN_FILENO, getpid());  // Restore shell as foreground process

                    if (in_fd != 0) close(in_fd);
                    if (out_fd != 1) close(out_fd);
                    if (error_fd != 2) close(error_fd);
                }
            }
        }
        free(prompt);
    }
    return 0;
}
