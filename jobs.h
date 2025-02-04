// jobs.h
#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 20
#define CMD_LEN 100

typedef struct Job {
    int jobId;
    pid_t pgid;      // Process Group ID
    char state[10];  // Running, Stopped, Done
    char command[CMD_LEN]; // Original command
} Job;

extern Job jobTable[MAX_JOBS]; // Declare job table
extern int jobCount;           // Declare job count

void addJob(pid_t pgid, const char *state, const char *command);
void updateJobState(pid_t pgid, const char *state);
void removeJob(pid_t pgid);
void removeDoneJobs();
void printJobs();
void fgCommand();
void bgCommand();
void jobsCommand();

#endif
