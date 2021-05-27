#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "sig.h"

int exitCode;
pid_t fgpid;
pid_t curbgpid;
pid_t prevbgpid;
int fgrun;
struct job bg_jobs[MAXJOBS];
struct job foreground;

/* get exit code when the process fq11inished */
void getExitStatus(int status) {

    if (WIFEXITED(status)) { 
        exitCode = WEXITSTATUS(status);
    }
    else { printf("\n"); } /* print new line if the child process terminate abnormally */
        
    if(WIFSIGNALED(status)) {
        exitCode = 128 + WTERMSIG(status);
    }
    if(WIFSTOPPED(status)) {
        exitCode = 128 + WSTOPSIG(status);
    }
}

void printJobDone(pid_t pid) {
    for(int i = 1; i<MAXJOBS; i++) {
        if (bg_jobs[i].pid == pid) {
            int isCur = 0;
            int isPrev = 0;
            if(bg_jobs[i].pid==curbgpid) isCur = 1;
            else if (bg_jobs[i].pid==prevbgpid) isPrev = 1;
            printf("\n[%d]%s%s\tDone\t\t%s\n", bg_jobs[i].job_id, isCur ? "+" : "" , isPrev ? "-" : "" , bg_jobs[i].command);
            deleteBgJob(pid, bg_jobs[i].job_id);
        }
    }
}

void child_handler (int signum) {
    pid_t pid;
    int status;

    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED))>0) {
        if (pid != fgpid) { /* if it is background job */
            printJobDone(pid);
        }
        else {
            getExitStatus(status);
            if(WIFSTOPPED(status)) {
                pid_t temp = curbgpid;
                curbgpid = pid;
                prevbgpid = temp;
                int jobID = findFreeJobID();
                printf("[%d]+\tStopped\t\t%s\n", jobID, foreground.command);
                addBgJob(pid, jobID, foreground.command, "Stopped"); /* send suspensed process to background job */
            }
            fgrun = 0; /* foreground job terminated */
        }
    }
}

void enableDefaultHandlers() {

    /* restore default sigaction */
    struct sigaction action;
    action.sa_handler = SIG_DFL;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTSTP, &action, NULL);
}

void enableSignalHandlers() {

    /* Install sig_child handler */
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = child_handler;

    sigaction(SIGCHLD, &action, NULL);

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    
    /* Control-C. ignore SIGINT in parent process. */
    sigaction(SIGINT, &sa, NULL);
    /* Control-Z. ignore SIGTSTP in parent process. */
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGTTOU, &sa, NULL);
}