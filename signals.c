#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern pid_t fg_pgid;  // Foreground process group

void handle_sigint(int signo) {
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGINT);  // Send SIGINT only to the foreground process group
    }
    write(STDOUT_FILENO, "\n# ", 3);
}

void handle_sigtstp(int signo) {
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGTSTP);  // Suspend the foreground process
    }
    write(STDOUT_FILENO, "\n# ", 3);
}

void handle_sigchld(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);  // Clean up zombie processes
}

void setup_signal_handlers() {
    signal(SIGINT, handle_sigint);   // Override CTRL+C
    signal(SIGTSTP, handle_sigtstp); // Override CTRL+Z
    signal(SIGCHLD, handle_sigchld); // Clean up background processes
    signal(SIGTTOU, SIG_IGN);        // Ignore terminal output for background jobs
}
