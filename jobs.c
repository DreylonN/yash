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

    // Find the next available job ID
    int jobId = 1;
    for (int i = 0; i < jobCount; i++) {
        if (jobTable[i].jobId >= jobId) {
            jobId = jobTable[i].jobId + 1;  // Assign the next available ID
        }
    }

    jobTable[jobCount].jobId = jobId;
    jobTable[jobCount].pgid = pgid;
    strncpy(jobTable[jobCount].state, state, sizeof(jobTable[jobCount].state) - 1);
    strncpy(jobTable[jobCount].command, command, CMD_LEN - 1);
    jobCount++;
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

void removeDoneJobs() {
    int shift = 0;
    
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(jobTable[i].state, "Done") == 0) {
            shift++;  // Count how many jobs to remove
        } else if (shift > 0) {
            jobTable[i - shift] = jobTable[i];  // Shift jobs up
        }
    }
    
    jobCount -= shift;  // Update job count
}

void printJobs() {
    // First, print "Done" jobs
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(jobTable[i].state, "Done") == 0) {
            printf("[%d]- Done\t\t%s\n", jobTable[i].jobId, jobTable[i].command);
        }
    }
    
    // Then, print Running/Stopped jobs
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(jobTable[i].state, "Done") != 0) {  // Skip "Done" jobs
            printf("[%d]%c %s\t\t%s\n",
                   jobTable[i].jobId,
                   (i == jobCount - 1) ? '+' : '-',
                   jobTable[i].state,
                   jobTable[i].command);
        }
    }
    
    // Remove "Done" jobs after printing
    removeDoneJobs();
}

void fgCommand() {
    if (jobCount == 0) {
        printf("No jobs to resume.\n");
        return;
    }

    Job *job = NULL;  // Get most recent job
    for (int i = jobCount - 1; i >= 0; i--) {
        if (strcmp(jobTable[i].state, "Stopped") == 0 || strcmp(jobTable[i].state, "Running") == 0) {
            job = &jobTable[i];
            break;
        }
    }

    if (job == NULL) {
        printf("No jobs to resume.\n"); // Prevents resuming completed jobs
        return;
    }

    // Remove '&' from command if it exists
    char *ampersand = strrchr(job->command, '&');
    if (ampersand && *(ampersand - 1) == ' ') {
        *(ampersand - 1) = '\0';  // Trim trailing space before '&'
    } else if (ampersand) {
        *ampersand = '\0';  // Remove '&'
    }
    
    printf("%s\n", job->command); // Print the command name

    tcsetpgrp(STDIN_FILENO, job->pgid);
    kill(-job->pgid, SIGCONT);  // Send SIGCONT to resume the job
    updateJobState(job->pgid, "Running");

    int status;
    waitpid(-job->pgid, &status, WUNTRACED);

    if (WIFSTOPPED(status)) {
        updateJobState(job->pgid, "Stopped");
    } else {
        removeJob(job->pgid);  // Remove completed jobs
    }
    
    tcsetpgrp(STDIN_FILENO, getpid()); // Restore shell control
}


void bgCommand() {
    for (int i = jobCount - 1; i >= 0; i--) {
        if (strcmp(jobTable[i].state, "Stopped") == 0) {
            // Correct background job output format
            printf("[%d]+ %s &\n", jobTable[i].jobId, jobTable[i].command);
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
