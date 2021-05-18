#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "sig.h"

void child_handler(int signum) {
    pid_t pid;
    int status;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) 
        ;
}

void sigtstp_handler(int signum) { printf("\nsuspensed"); }

void enableSignalHandlers() {

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = child_handler;

    sigaction(SIGCHLD, &sa, NULL);

    // ignore SIGINT in parent process.
    struct sigaction sigint1, sigint2;
    sigint1.sa_handler = SIG_IGN;
    sigemptyset(&sigint1.sa_mask);
    sigint1.sa_flags = 0;

    sigaction(SIGINT, NULL, &sigint2);
    if (sigint2.sa_handler != SIG_IGN)
        sigaction(SIGINT, &sigint1, NULL);

    // Install SIGTSTP handler
    struct sigaction sigtstp;
    sigtstp.sa_handler = sigtstp_handler;
    sigemptyset(&sigtstp.sa_mask);
    sigtstp.sa_flags = 0;

    sigaction(SIGTSTP, &sigtstp, NULL);
}
