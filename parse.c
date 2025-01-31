#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Parses the input using and stores tokens in args
// Returns the number of arguments parsed
int parseInput(const char *prompt, char args[][30], int max_args) {
    if (prompt == NULL || strlen(prompt) == 0) {
        return 0; // No valid input
    }

    int count = 0;
    char prompt_copy[2000]; // Copy input for tokenization
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

    // Invalid: Commands starting with <, >, 2>, |, &
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