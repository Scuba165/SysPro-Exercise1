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


#include "../lib/job.h"
#include "../lib/queue.h"
#define MSGSIZE 100

// Pipe initialization(both ways).
char* comToServer = "/tmp/comToServer";
char* serverToCom = "/tmp/serverToCom";
int serverComFd;

int main (int argc, char* argv[]) {
    pid_t serverPid;
    int fd, i, nwrite;
    char msgbuf[MSGSIZE + 1] = "\0";

    pid_t commanderPid = getpid();
    char* buf = malloc(sizeof(commanderPid));
    sprintf(buf, "%d", commanderPid); // Convert pid to string and write it so we can signal back.
    strcat(msgbuf, buf);
    strcat(msgbuf, " ");
    free(buf);

    if (argc < 2) { 
        printf(" Usage: ./jobCommander <command> <arguments> \n"); 
        exit(1); 
    }

    if(access("jobExecutorServer.txt", F_OK) != 0) { // Check if server is active.
        if((serverPid = fork()) == 0) { // Run jobExecutorServer if child.
            char *args[] = {"./jobExecutorServer", "&", NULL}; 
            execvp(args[0], args);
        } else {    
            int status;
            waitpid(serverPid, &status, 0); // Wait for server(child) to be set-up.
            if(status != 0) {
                perror("Waitpid-Server setup:");
            }
        }
    }

    int textFile;
    if((textFile = open("jobExecutorServer.txt", O_RDONLY | O_NONBLOCK)) < 0) {
        perror("Commander: Text file reading");
        exit(6);
    }
    char pidbuf[64];
    if((read(textFile, pidbuf, 64)) < 0) {
        perror("Error reading pid");
        close(textFile);
        exit(5);
    }
    serverPid = atoi(pidbuf);
    close(textFile);

    // Write job in buffer.
    for (i = 1; i < argc; i++) {
        strcat(msgbuf, argv[i]);
        strcat(msgbuf, " ");
    }

    if ((fd = open(comToServer, O_RDWR | O_NONBLOCK)) < 0) { // Open existing pipe for sending jobs to the server.
        perror("Commander: Com to Server open error:"); 
        exit(1);
    }

    // Write job into pipe
    if ((nwrite = write(fd, msgbuf, MSGSIZE + 1)) == -1) { 
        perror("Error in Writing");
        exit(2);
    }
    close(fd);


    if ((serverComFd = open(serverToCom, O_RDONLY | O_NONBLOCK)) < 0) { // Open existing pipe for receiving server feedback.
        perror("Commander: Server to Com open error:"); 
        exit(1);
    }
    kill(serverPid, SIGUSR1); // Signal to server that a job has been sent.
    for(;;) {
        signal(SIGUSR2, serverReturn); // Retrieve signal to print feedback.
    }
    exit(0);
}

void serverReturn(int signum) {
    char returnBuf[64];
    int bytes;
    int written = 0;
    while((bytes = read(serverComFd, returnBuf, 63)) > 0) { // Read pipe and print.
        returnBuf[bytes] = '\0';
        printf("%s", returnBuf);
        written = 1;
    }
    if(written) printf("\n");
    close(serverComFd);
    exit(0); // Done.
}