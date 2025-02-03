#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_JOBS 20
#define CMD_LEN 100

typedef struct Job {
    int jobId;
    pid_t pgid;
    char state[10]; // Running, Stopped, Done
    char command[CMD_LEN];
} Job;

Job jobTable[MAX_JOBS]; // Global job table
int jobCount = 0;       // Number of active jobs

void addJob(pid_t pgid, const char *state, const char *command) {
    if (jobCount >= MAX_JOBS) {
        fprintf(stderr, "Error: Maximum job limit reached.\n");
        return;
    }
    jobTable[jobCount].jobId = jobCount + 1;
    jobTable[jobCount].pgid = pgid;
    strncpy(jobTable[jobCount].state, state, sizeof(jobTable[jobCount].state) - 1);
    strncpy(jobTable[jobCount].command, command, CMD_LEN - 1);
    jobCount++;
}

const char *getJobCommand(pid_t pgid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobTable[i].pgid == pgid) {
            return jobTable[i].command;
        }
    }
    return "";  // Return empty string if no job found
}

int jobExists(pid_t pgid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobTable[i].pgid == pgid) {
            return 1;
        }
    }
    return 0;
}

void updateJobState(pid_t pgid, const char *state) {
    for (int i = 0; i < jobCount; i++) {
        if (jobTable[i].pgid == pgid) {
            strncpy(jobTable[i].state, state, sizeof(jobTable[i].state) - 1);
            break;
        }
    }
}

void removeJob(pid_t pgid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobTable[i].pgid == pgid) {
            for (int j = i; j < jobCount - 1; j++) {
                jobTable[j] = jobTable[j + 1];
            }
            jobCount--;
            break;
        }
    }
}

void printJobs() {
    int i = 0;
    while (i < jobCount) {
        printf("[%d] %c %s\t%s\n",
               jobTable[i].jobId,
               (i == jobCount - 1) ? '+' : '-',
               jobTable[i].state,
               jobTable[i].command);

        // Remove "Done" jobs **after** displaying them
        if (strcmp(jobTable[i].state, "Done") == 0) {
            removeJob(jobTable[i].pgid);
        } else {
            i++;  // Only increment if not removed
        }
    }
}

void fgCommand() {
    if (jobCount == 0) {
        printf("No jobs to resume.\n");
        return;
    }
    Job *job = &jobTable[jobCount - 1];
    printf("%s\n", job->command);

    tcsetpgrp(STDIN_FILENO, job->pgid);
    kill(-job->pgid, SIGCONT);  // Send SIGCONT to the job
    updateJobState(job->pgid, "Running");

    int status;
    waitpid(-job->pgid, &status, WUNTRACED);

    if (WIFSTOPPED(status)) {
        updateJobState(job->pgid, "Stopped");
    } else {
        removeJob(job->pgid);  // Remove completed jobs
    }
    tcsetpgrp(STDIN_FILENO, getpid()); // Restore shell
}

void bgCommand() {
    for (int i = jobCount - 1; i >= 0; i--) {
        if (strcmp(jobTable[i].state, "Stopped") == 0) {
            printf("[%d] + %s &\n", jobTable[i].jobId, jobTable[i].command);
            kill(-jobTable[i].pgid, SIGCONT);  // Resume job
            updateJobState(jobTable[i].pgid, "Running");
            return;
        }
    }
    printf("No stopped jobs to resume.\n");
}

void jobsCommand() {
    printJobs();
}
