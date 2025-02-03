#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobs.h"
#include <string.h>

extern pid_t fg_pgid;  // Foreground process group

void handle_sigint(int signo) {
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGINT);  // Send SIGINT only to the foreground process group
    }
    write(STDOUT_FILENO, "\n# ", 3);
}

void handle_sigtstp(int signo) {
    printf("DEBUGGING: fg_pgid: %d", fg_pgid);
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGTSTP);  // Suspend the foreground process
    }
    write(STDOUT_FILENO, "\n# ", 3);
}


void handle_sigchld(int signo) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            updateJobState(pid, "Done");
        } else if (WIFSTOPPED(status)) {
            updateJobState(pid, "Stopped");
        }
    }
}


void handle_sigcont(int signo) {
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(jobTable[i].state, "Stopped") == 0) {
            updateJobState(jobTable[i].pgid, "Running");
        }
    }
}

void setup_signal_handlers() {
    signal(SIGINT, handle_sigint);      // Handle CTRL+C
    signal(SIGTSTP, handle_sigtstp);   // Handle CTRL+Z
    signal(SIGCHLD, handle_sigchld);   // Handle child state changes
    signal(SIGCONT, handle_sigcont);   // Handle job continuation
    signal(SIGTTOU, SIG_IGN);          // Ignore background output issues
}

