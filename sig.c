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

void enableDefaultHandlers() {

    // restore default sigaction
    struct sigaction action;
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, NULL);

    struct sigaction sigtstp;
    sigtstp.sa_handler = SIG_DFL;
    sigemptyset(&sigtstp.sa_mask);
    sigtstp.sa_flags = 0;

    sigaction(SIGTSTP, &sigtstp, NULL);
}

void enableSignalHandlers() {

    // Install sig_child handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = child_handler;

    sigaction(SIGCHLD, &sa, NULL);

    // Control-C. ignore SIGINT in parent process.
    struct sigaction sigint1, sigint2;
    sigint1.sa_handler = SIG_IGN;
    sigemptyset(&sigint1.sa_mask);
    sigint1.sa_flags = 0;

    sigaction(SIGINT, NULL, &sigint2);
    if (sigint2.sa_handler != SIG_IGN)
        sigaction(SIGINT, &sigint1, NULL);

    // Control-Z. ignore SIGTSTP in parent process.
    struct sigaction sigtstp;
    sigtstp.sa_handler = SIG_IGN;
    sigemptyset(&sigtstp.sa_mask);
    sigtstp.sa_flags = 0;

    sigaction(SIGTSTP, &sigtstp, NULL);
}
