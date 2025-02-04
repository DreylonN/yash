#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

extern pid_t fg_pgid;  // Foreground process group

// Handles redirection based on <, >, and 2> tokens
int redirectionHandler(char *args[], int *in_fd, int *out_fd, int *error_fd) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            // Input redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for input redirection\n");
                return 1;
            }
            *in_fd = open(args[i + 1], O_RDONLY); //checks if file exists
            if (*in_fd == -1) {
                perror(args[i + 1]);
                return 1;
            }
            args[i] = NULL; // Remove redirection from args
        } else if (strcmp(args[i], ">") == 0) {
            // Output redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for output redirection\n");
                return 1;
            }
            *out_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644); //checks if file exists
            if (*out_fd == -1) {
                perror(args[i + 1]);
                return 1;
            }
            args[i] = NULL; // Remove redirection from args
        } else if (strcmp(args[i], "2>") == 0) {
            // Error redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for error redirection\n");
                return 1;
            }
            *error_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644); //checks if file exists
            if (*error_fd == -1) {
                perror(args[i + 1]);
                return 1;
            }
            args[i] = NULL; // Remove redirection from args
        }
    }
    return 0;
}

// Handles piping
void pipeHandler(char *args[]) {
    int pipefd[2];
    char *leftArgs[25], *rightArgs[25];
    int leftCount = 0, rightCount = 0;
    int leftIn_fd = -1, leftOut_fd = -1, leftError_fd = -1;
    int rightIn_fd = -1, rightOut_fd = -1, rightError_fd = -1;
    int i, foundPipe = -1;
    pid_t pgid = 0; // Process Group ID

    // Find the pipe symbol |
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            foundPipe = i;
            break;
        }
    }
    if (foundPipe == -1) return; // No pipe found, return.

    // Split the command at |
    for (i = 0; i < foundPipe; i++) leftArgs[leftCount++] = args[i];
    leftArgs[leftCount] = NULL;

    for (i = foundPipe + 1; args[i] != NULL; i++) rightArgs[rightCount++] = args[i];
    rightArgs[rightCount] = NULL;

    // Apply redirection to both left and right commands
    int leftRedirectErr = redirectionHandler(leftArgs, &leftIn_fd, &leftOut_fd, &leftError_fd);
    int rightRedirectErr = redirectionHandler(rightArgs, &rightIn_fd, &rightOut_fd, &rightError_fd);
    
    if (leftRedirectErr || rightRedirectErr) return; // Return if redirection fails

    // Creates pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    // First process (left command)
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid1 == 0) {
        setpgid(0, 0);  // Set itself as PGID leader (creates a new process group)
        if (leftIn_fd != -1) dup2(leftIn_fd, STDIN_FILENO);  // Redirect input if specified.
        if (leftOut_fd == -1) dup2(pipefd[1], STDOUT_FILENO); // Only redirect to pipe if no file output is set
        else dup2(leftOut_fd, STDOUT_FILENO); // If file output (`> file`), redirect to it.
        if (leftError_fd != -1) dup2(leftError_fd, STDERR_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);

        execvp(leftArgs[0], leftArgs);
        perror("Left command execution failed");
        exit(1);
    }

    pgid = pid1; // Store PGID from the first process
    fg_pgid = pgid; // Set the foreground process group to the first process

    // **Second process (right command).**
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid2 == 0) {
        setpgid(0, pgid); // Join the process group of the first process

        if (rightIn_fd == -1) dup2(pipefd[0], STDIN_FILENO); // Only use pipe input if no `< file` is set.
        else dup2(rightIn_fd, STDIN_FILENO); // If `< file` is set, use it instead.
        if (rightOut_fd != -1) dup2(rightOut_fd, STDOUT_FILENO);
        if (rightError_fd != -1) dup2(rightError_fd, STDERR_FILENO);

        close(pipefd[1]);
        close(pipefd[0]);

        execvp(rightArgs[0], rightArgs);
        perror("Right command execution failed");
        exit(1);
    }

    // Set both processes in the same PGID
    setpgid(pid2, pgid);

    tcsetpgrp(STDIN_FILENO, pgid); // Set terminal control to the process group

    close(pipefd[0]);
    close(pipefd[1]);

    int status1, status2;
    waitpid(pid1, &status1, WUNTRACED);
    waitpid(pid2, &status2, WUNTRACED);

    if (WIFSTOPPED(status1) || WIFSTOPPED(status2)) {
        printf("\nDEBUGGING: Pipe process group stopped.\n");
    }

    tcsetpgrp(STDIN_FILENO, getpid()); // Restore shell as foreground process
}