#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>

int parseInput(const char *input, char args[][30], int maxArgs);
void redirectionHandler(char *args[], int *in_fd, int *out_fd, int *error_fd);
void pipeHandler(char *args[]);
int isValidCommand(char *args[]);

int main() {
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

            // **Check for Invalid Commands**
            if (!isValidCommand(argv)) {
                free(prompt);
                continue; // Ignore and print new prompt
            }

            // **Check if command contains a pipe**
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
                redirectionHandler(argv, &in_fd, &out_fd, &error_fd);

                // DEBUG: Print arguments
                // for (int i = 0; argv[i] != NULL; i++) {
                //     printf("argv[%d]: %s\n", i, argv[i]);
                // }

                pid_t pid = fork();
                if (pid < 0) {
                    perror("Fork failed");
                } else if (pid == 0) {
                    if (in_fd != 0) dup2(in_fd, STDIN_FILENO);
                    if (out_fd != 1) dup2(out_fd, STDOUT_FILENO);
                    if (error_fd != 2) dup2(error_fd, STDERR_FILENO);

                    //DEBUG: Prints fds
                    // printf("Input redirected to file descriptor %d\n", in_fd);
                    // printf("Output redirected to file descriptor %d\n", out_fd);
                    // printf("Error redirected to file descriptor %d\n", error_fd);

                    execvp(argv[0], argv);
                    perror("CommanSd execution failed");
                    exit(1);
                } else {
                    int status;
                    waitpid(pid, &status, 0);
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
