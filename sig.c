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

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTSTP, &action, NULL);
}

void enableSignalHandlers() {

    // Install sig_child handler
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = child_handler;

    sigaction(SIGCHLD, &action, NULL);

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    
    // Control-C. ignore SIGINT in parent process.
    sigaction(SIGINT, &sa, NULL);
    // Control-Z. ignore SIGTSTP in parent process.
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGTTOU, &sa, NULL);
}