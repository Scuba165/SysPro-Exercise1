#include "../lib/queue.h"
#pragma once
// Header file for jobCommander and jobExecutorServer.

// Struct job.
typedef struct job {
    char id[8];
    int queuePos;
    char jobBuffer[100];
} Job;

// Terminates all running jobs and deletes .txt file.
void serverExit();

// Create Job struct and insert into waitingJobs queue.
int enqueueJob(char* buffer);

// List all jobs in given queue. 
// If flag is 0, QUEUE POSITION is printed as a header, if flag is 1, PROCCESS ID is printed instead.
int listAllJobs(Queue queue, int flag);

// Use job_XX string to find a job in a queue and return it.
Job* findJob(Queue queue, char* stopId);

// Use waitpid() to find finished jobs and dequeue them.
void dequeueFinishedJob(int signum);

// Read pipe after server has written and print output.
void serverReturn(int signum);

// Read job from pipe and act accordingly.
void executeCommand(int signum);