#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "../lib/job.h"
#include "../lib/queue.h"
#define MSGSIZE 100

// Pipe initialization(both ways).
char* comToServer = "/tmp/comToServer";
char* serverToCom = "/tmp/serverToCom";
int comServerFd, serverComFd, textFile;
char msgbuf[MSGSIZE + 1];
int serverConcurrency = 1;
Queue runningJobs, waitingJobs;
int jobID = 1;

int main(int argc, char* argv[]) {

    if (argc > 2) {
        printf("Usage: ./jobExecutorServer &\n");
        exit(1);
    }

    // Create pipes for communication between commanders-server.
    if (mkfifo(comToServer, 0666) == -1) {
        if (errno != EEXIST ) {
            perror("Commander to server pipe creation:") ;
            exit(6);
        }
    }
    if (mkfifo(serverToCom, 0666) == -1) {
        if (errno != EEXIST ) {
            perror("Server to commander pipe creation:") ;
            exit(6);
        }
    }

    // Open pipe for receiving commands.
    if ((comServerFd = open(comToServer , O_RDONLY | O_NONBLOCK)) < 0) {
        perror ("comToServer open problem");
        exit(3);
    }

    // Fork. Parent proccess writes in txt file and returns to possibly unblock waitpid() in commander. Child runs 
    // the server until shut down.
    int txtFork;
    if((txtFork = fork()) > 0) {
        if((textFile = open("jobExecutorServer.txt", O_RDWR | O_CREAT, 0644)) < 0) {
            perror("Server: Text file creation:");
            exit(6);
        } else {     
            pid_t serverPid = txtFork;
            char* buf = malloc(sizeof(serverPid));
            sprintf(buf, "%d", serverPid); // Convert pid to string and write in .txt
            ssize_t bytes;
            if(((bytes = write(textFile, buf, strlen(buf))) == -1)) {
                close(textFile);
                perror("Error in file writing");
                exit(1);
            }
            printf("Server is active with pid %d.\n", serverPid);
            close(textFile);
            free(buf);
            exit(0); // Parent stops here.
        }
    }

    runningJobs = queue_create();
    waitingJobs = queue_create();
    
    for (;;) {
        signal(SIGUSR1, executeCommand);
        signal(SIGCHLD, dequeueFinishedJob);
        while((queue_length(runningJobs) < serverConcurrency) && (queue_length(waitingJobs) != 0)) {
            Job* job = malloc(sizeof(job));
            job = (Job*)queue_node_value(waitingJobs, queue_first(waitingJobs));
            
            // Update queue positions in waitingJobs.
            for(QueueNode node = queue_first(waitingJobs); node != NULL; node = queue_next(waitingJobs, node)) {
                Job* waitPos = queue_node_value(waitingJobs, node);
                waitPos->queuePos--;
            }
            int child;
            if((child = fork()) == 0) {
                char* command[100];
                char* token = strtok(job->jobBuffer, " ");
                int tok_num = 0;
                // Store tokens in the array
                while (token != NULL) {
                    command[tok_num] = token;
                    tok_num++;
                    token = strtok(NULL, " ");
                }
                command[tok_num] = NULL; // NULL terminate the string for execvp.
                execvp(command[0], command);
                perror("Execvp job execution:");
                exit(6);
            } else {
                job->queuePos = child; // Update queuePos. It ressembles PID in active jobs.
                if(queue_insert_last(runningJobs, job) != 0) {
                    perror("Running jobs queue insert:");
                }
                if(queue_remove_first(waitingJobs) != 0) {
                    perror("Waiting jobs queue remove:");
                }
            }
        }
        
    }
}


void executeCommand(int signum) {

    if(read(comServerFd, msgbuf, MSGSIZE + 1) < 0) {
        perror("problem in reading");
        exit(5);
    }

    // Pipe has been oppened as O_RDONLY so we can now open the pipe to write back.
    if((serverComFd = open(serverToCom, O_RDWR | O_NONBLOCK)) < 0) { 
        perror("Server: Server to Com open error:"); 
        exit(1);
    }

    // Tokenise the string so we have the PID of the commander, the command and the rest of the job.
    char* commanderPidStr = strtok(msgbuf, " ");
    int commanderPid = atoi(commanderPidStr);
    char* commanderArg = strtok(NULL, " ");
    char* allJob = strtok(NULL, "\0");

    // 5 cases for each job type.
    pid_t children;
    if(strcmp(commanderArg, "issueJob") == 0) {
        int currID = jobID;
        int queuePos = enqueueJob(allJob);
        jobID++;
        char* buf2 = malloc(100);
        sprintf(buf2, "<job_%d, %s, %d>", currID, allJob, queuePos);
        ssize_t bytes;
        if((bytes = write(serverComFd, buf2, strlen(buf2))) == -1) { 
            perror("Server: Server to Com writing eror:");
            exit(1);
        }
        free(buf2);
        kill(commanderPid, SIGUSR2); // Signal commander to print <ID, Job, QueuePos>
    }
    else if(strcmp(commanderArg, "setConcurrency") == 0) {
        serverConcurrency = atoi(strtok(allJob, " "));
        kill(commanderPid, SIGUSR2); // Signal commander so he can exit.
    }
    else if(strcmp(commanderArg, "poll") == 0) {
        char* nextArg = strtok(allJob, " ");
        if(strcmp(nextArg, "running") == 0) {
            listAllJobs(runningJobs, 1); // flag = 1 printf will use "PID"
        } else if(strcmp(nextArg, "queued") == 0) {
            listAllJobs(waitingJobs, 0); // flag = 0 printf will use "QUEUE POSITION"
        } else {
            printf("Incorrect input! Command ignored.\n");
        }
        kill(commanderPid, SIGUSR2);
    }
    else if(strcmp(commanderArg, "stop") == 0) {
        char* jobID = strtok(allJob, " ");
        Job* search;
        char* jobStatus;
        if((search = findJob(runningJobs, jobID)) != NULL) {
            if (kill(search->queuePos, SIGTERM) == -1) {
                perror("kill");
                exit(EXIT_FAILURE);
            }
            queue_remove_value(runningJobs, search);
            jobStatus = "terminated";
        } else if((search = findJob(waitingJobs, jobID)) != NULL) {
            queue_remove_value(waitingJobs, search);
            jobStatus = "removed";
        } else {
            printf("%s not found. Command \"stop\" ignored.\n", jobID);
            close(serverComFd);   
            kill(commanderPid, SIGUSR2);
            return;
        }
        char* jobTriple = malloc(100);
        sprintf(jobTriple, "%s %s\n", search->id, jobStatus);
        int bytes;
        if((bytes = write(serverComFd, jobTriple, strlen(jobTriple))) == -1) {
            perror("Server: Server to Com writing eror:");
        }
        free(jobTriple);
        kill(commanderPid, SIGUSR2);
    }
    else if(strcmp(commanderArg, "exit") == 0) {
        if((children = fork()) == 0) {
            serverExit();
        } else {
            ssize_t bytes;
            char* buf2 = "jobExecutorServer terminated.";
            if((bytes = write(serverComFd, buf2, strlen(buf2))) == -1) {
                perror("Server: Server to Com writing eror:");
                exit(1);
            }
            kill(commanderPid, SIGUSR2);
            exit(0);
        }
        
    } else {
        printf("Incorrect input! Command ignored.\n");
        kill(commanderPid, SIGUSR2);
    } 
    close(serverComFd);      
}


void serverExit() {
    QueueNode node;
    for(node = queue_first(runningJobs); node != NULL; node = queue_next(runningJobs, node)) {
        Job* running = queue_node_value(runningJobs, node);
        kill(running->queuePos, SIGTERM);
    }
    queue_destroy(waitingJobs);
    queue_destroy(runningJobs);
    char* args[] = {"rm", "-f", "jobExecutorServer.txt", NULL};
    execvp(args[0], args);
    perror("execvp");
}

int enqueueJob(char* buff) {
    Job* new = malloc(sizeof(*new)); 
    strcpy(new->jobBuffer, buff);
    sprintf(new->id, "job_%d", jobID);
    new->queuePos = queue_length(waitingJobs) + 1;
    queue_insert_last(waitingJobs, new);
    return new->queuePos;
}


void dequeueFinishedJob(int signum) {
	pid_t pid;
    int stat;

	while ((pid = waitpid((pid_t)(-1), &stat, WNOHANG)) > 0) { // Wait for children that finished and remove pid from queue.
		queue_remove_pid(runningJobs, pid);
    }
    return;
}


int listAllJobs(Queue queue, int flag) {
    int bytes;
    if(queue_length(queue) == 0) {
        char* empty = "There are currently no jobs waiting.";
        if(flag) empty = "There are currently no jobs running.";
        if((bytes = write(serverComFd, empty, strlen(empty))) == -1) {
            perror("Server: Server to Com writing eror:");
            return 1;
        }
        return 0;
    }
    char* header = "  ID  |    COMMAND    |     QUEUE POSITION\n";
    // If flag is 1, we are listing running jobs
    if(flag == 1) {
        header = "  ID  |    COMMAND    |     PROCCESS ID\n";
    }
    if((bytes = write(serverComFd, header, strlen(header))) == -1) {
            perror("Server: Server to Com writing eror:");
            return 1;
        }
    for(QueueNode node = queue_first(queue); node != NULL; node = queue_next(queue, node)) {
        Job* job = queue_node_value(queue, node);
        char* jobTriple = malloc(100);
        sprintf(jobTriple, "<%s, %s, %d>\n", job->id, job->jobBuffer, job->queuePos);
        if((bytes = write(serverComFd, jobTriple, strlen(jobTriple))) == -1) {
            perror("Server: Server to Com writing eror:");
            return 1;
        }
        free(jobTriple);
    }
    return 0;
}

Job* findJob(Queue queue, char* stopId) {
    for(QueueNode node = queue_first(queue); node != NULL; node = queue_next(queue, node)) {
        Job* job = queue_node_value(queue, node);
        if(strcmp(job->id, stopId) == 0) {
            return job;
        }
    }
    return NULL;
}