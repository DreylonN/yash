#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>

int parse_input(const char *input, char args[][30], int max_args);
void handle_redirection(char *args[], int *stdin_fd, int *stdout_fd, int *stderr_fd);

int main() {
    while (1) {
        char *prompt = readline("# ");
        if (prompt == NULL) {
            exit(0); // Exit the shell on EOF
        }

        const int max_args = 50;
        char args[max_args][30];
        int arg_count = parse_input(prompt, args, max_args);

        if (arg_count > 0) {
            // Prepare the argument array for execvp
            char *argv[max_args + 1];
            for (int i = 0; i < arg_count; i++) {
                argv[i] = args[i];
            }
            argv[arg_count] = NULL;

            int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;

            // Handle file redirection
            redirection(argv, &stdin_fd, &stdout_fd, &stderr_fd);

            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork failed");
            } else if (pid == 0) {
                // Apply redirection if applicable
                if (stdin_fd != -1) dup2(stdin_fd, STDIN_FILENO);
                if (stdout_fd != -1) dup2(stdout_fd, STDOUT_FILENO);
                if (stderr_fd != -1) dup2(stderr_fd, STDERR_FILENO);

                // Execute the command
                execvp(argv[0], argv);
                perror("Command execution failed");
                exit(1);
            } else {
                // Wait for the child process
                int status;
                waitpid(pid, &status, 0);

                // Close redirection file descriptors
                if (stdin_fd != -1) close(stdin_fd);
                if (stdout_fd != -1) close(stdout_fd);
                if (stderr_fd != -1) close(stderr_fd);
            }
        }

        free(prompt);
    }

    return 0;
}
