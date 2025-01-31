#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Parses the input using strtok() and stores tokens in the args array.
// Returns the number of arguments parsed, up to max_args.
int parseInput(const char *prompt, char args[][30], int max_args) {
    if (prompt == NULL || strlen(prompt) == 0) {
        return 0; // No valid input
    }

    int count = 0;
    char prompt_copy[2000]; // Copy input forS tokenization
    strncpy(prompt_copy, prompt, sizeof(prompt_copy) - 1);
    prompt_copy[sizeof(prompt_copy) - 1] = '\0'; // Ensure null-termination

    char *token = strtok(prompt_copy, " ");
    while (token != NULL && count < max_args) {
        // Copy token into args array
        strncpy(args[count], token, 29);
        args[count][29] = '\0'; // Ensure null-termination
        count++;
        token = strtok(NULL, " ");
    }

    return count; // Return the number of arguments parsed
}

int isValidCommand(char *args[]) {
    if (args[0] == NULL) return 0; // Empty command

    // Invalid: Commands starting with <, >, 2>, |, &**
    if (strcmp(args[0], "<") == 0 || 
        strcmp(args[0], ">") == 0 || 
        strcmp(args[0], "2>") == 0 || 
        strcmp(args[0], "|") == 0 || 
        strcmp(args[0], "&") == 0) {
        return 0; // Invalid command
    }
    // Valid Command
    return 1;
}

// Handles redirection based on <, >, and 2> tokens
void redirectionHandler(char *args[], int *in_fd, int *out_fd, int *error_fd) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            // Input redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for input redirection\n");
                exit(1);
            }
            *in_fd = open(args[i + 1], O_RDONLY); //checks if file exists
            if (*in_fd == -1) {
                perror("Error opening input file");
                exit(1);
            }
            args[i] = NULL; // Remove redirection from args
        } else if (strcmp(args[i], ">") == 0) {
            // Output redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for output redirection\n");
                exit(1);
            }
            *out_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644); //checks if file exists
            if (*out_fd == -1) {
                perror("Error opening output file");
                exit(1);
            }
            args[i] = NULL; // Remove redirection from args
        } else if (strcmp(args[i], "2>") == 0) {
            // Error redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for error redirection\n");
                exit(1);
            }
            *error_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644); //checks if file exists
            if (*error_fd == -1) {
                perror("Error opening error file");
                exit(1);
            }
            args[i] = NULL; // Remove redirection from args
        }
    }
}

// Handles piping
void pipeHandler(char *args[]) {
    int pipefd[2];
    char *leftArgs[25], *rightArgs[25];
    int leftCount = 0, rightCount = 0;
    int leftIn_fd = -1, leftOut_fd = -1, leftError_fd = -1;
    int rightIn_fd = -1, rightOut_fd = -1, rightError_fd = -1;
    int i, foundPipe = -1;

    // **Find the pipe symbol ("|").**
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            foundPipe = i;
            break;
        }
    }
    if (foundPipe == -1) return; // No pipe found, return.

    // **Split the command at "|".**
    for (i = 0; i < foundPipe; i++) leftArgs[leftCount++] = args[i];
    leftArgs[leftCount] = NULL;

    for (i = foundPipe + 1; args[i] != NULL; i++) rightArgs[rightCount++] = args[i];
    rightArgs[rightCount] = NULL;

    // **Apply redirection to both left and right commands.**
    redirectionHandler(leftArgs, &leftIn_fd, &leftOut_fd, &leftError_fd);
    redirectionHandler(rightArgs, &rightIn_fd, &rightOut_fd, &rightError_fd);

    // **Create pipe.**
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    pid_t pid1, pid2;

    // **First process (left command).**
    pid1 = fork();
    if (pid1 < 0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid1 == 0) {
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

    // **Second process (right command).**
    pid2 = fork();
    if (pid2 < 0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid2 == 0) {
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

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}