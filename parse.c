#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

// Parses the input using strtok() and stores tokens in the args array.
// Returns the number of arguments parsed, up to max_args.
int parse_input(const char *prompt, char args[][30], int max_args) {
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

// Handles redirection based on <, >, and 2> tokens
void redirection(char *args[], int *stdin_fd, int *stdout_fd, int *stderr_fd) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            // Input redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Error: Missing file for input redirection\n");
                exit(1);
            }
            *stdin_fd = open(args[i + 1], O_RDONLY);
            if (*stdin_fd == -1) {
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
            *stdout_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*stdout_fd == -1) {
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
            *stderr_fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*stderr_fd == -1) {
                perror("Error opening error file");
                exit(1);
            }
            args[i] = NULL; // Remove redirection from args
        }
    }
}

