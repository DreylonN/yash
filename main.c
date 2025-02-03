#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <signal.h>

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

            // Checks if fg command is entered and a process is stopped
            if (strcmp(argv[0], "fg") == 0) {
                if (fg_pgid > 0) {
                    printf("Resuming process %d\n", fg_pgid);
                    tcsetpgrp(STDIN_FILENO, fg_pgid);   // Set the foreground process group
                    kill(-fg_pgid, SIGCONT);  // Resume the stopped process
                    int status;
                    while (waitpid(-fg_pgid, &status, WUNTRACED) > 0); // Wait for all pipeline processes
                    tcsetpgrp(STDIN_FILENO, getpid()); // Restore shell
                } else {
                    printf("No process to resume\n");
                }
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
                        printf("\nProcess stopped. Use 'fg' to resume.\n");
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
