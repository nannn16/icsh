#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include "sig.h"

#define MAXLINE 1000

/* 
Reference: 
sigchld.c, sig_block.c
https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
https://stackoverflow.com/questions/32708086/ignoring-signals-in-parent-process
https://stackoverflow.com/questions/6335730/zombie-process-cant-be-killed
https://stackoverflow.com/questions/11515399/implementing-shell-in-c-and-need-help-handling-input-output-redirection
https://emersion.fr/blog/2019/job-control/
https://github.com/liang810612/Mini-Shell-Program/blob/master/msh.c
https://gist.github.com/anonymous/a139649e699da5ee4ae1#file-shellproject-c
*/

int exitCode;
int isIn;
int isOut;
pid_t fgpid;
pid_t curbgpid; /* + flag in job control */
pid_t prevbgpid; /* - flag in job control */
int fgrun; /* is foreground job still running? */
struct job bg_jobs[MAXJOBS];
struct job foreground;

int dup2(int fildes, int fildes2);

void ioRedirection(char *file) {

    if (isIn) {
        isIn = 0;
        int in;
        in = open (file, O_RDONLY);
        if (in <= 0) {
            fprintf (stderr, "Couldn't open a file\n");
            exit (errno);
        }
        dup2 (in, 0);
        close (in);
    }
    else if (isOut) {
        isOut = 0;
        int out;
        out = open (file, O_TRUNC | O_CREAT | O_WRONLY, 0666);
        dup2 (out, 1);
        close (out);
    }
}

/*  
    put words in array of input into array of pointer ptr
    to access it easier.
*/
void makeDoublePointer(char *ptr[], char input[]) {
    const char s[2] = " \n";
    char *token = strtok(input, s);
    int i = 0;
    while(token != NULL) {
        ptr[i++] = token;
        token = strtok(NULL, s);
    }
}

void updateFg(job fg, pid_t pid, int job_id, char *cmd, char *state) {
    fg.pid = pid;
    fg.job_id = job_id;
    strcpy(fg.command, cmd);
    strcpy(fg.state, state);
}

void addBgJob(pid_t pid, int job_id, char *cmd, char *state) {
    job process = { pid, job_id, NULL, NULL };
    process.command = malloc(1000*sizeof(char));
    process.state = malloc(10*sizeof(char));
    strcpy(process.command, cmd);
    strcpy(process.state, state);
    bg_jobs[job_id] = process;
}

void deleteBgJob(pid_t pid, int job_id) {
    if(bg_jobs[job_id].pid == pid) {
        bg_jobs[job_id].pid = 0;
        bg_jobs[job_id].job_id = 0;
        free(bg_jobs[job_id].command);
        free(bg_jobs[job_id].state);
    }
}

void printJobsList() {
    for(int i=0; i<MAXJOBS; i++) {
        if(bg_jobs[i].job_id>0) {
            job bg = bg_jobs[i];
            int isCur = 0;
            int isPrev = 0;
            if(bg.pid==curbgpid) isCur = 1;
            else if (bg.pid==prevbgpid) isPrev = 1;
            if(strcmp(bg.state, "Stopped")==0) {
                printf("[%d]%s%s\t%s\t\t%s\n", bg.job_id, isCur ? "+" : "" , isPrev ? "-" : "" , bg.state, bg.command);
            }
            else {
                printf("[%d]%s%s\t%s\t\t%s&\n", bg.job_id, isCur ? "+" : "" , isPrev ? "-" : "" , bg.state, bg.command);
            }
        }
    }
}

int findFreeJobID() {
    for(int i=1; i<MAXJOBS; i++) {
        if(bg_jobs[i].job_id==0) {
            return i;
        }
    }
    return 0;
}

void fg(int job_id) {
    pid_t pid = bg_jobs[job_id].pid;
    printf("%s\n", bg_jobs[job_id].command);
    updateFg(foreground, pid, job_id, bg_jobs[job_id].command, "foreground");
    deleteBgJob(pid, job_id);

    tcsetpgrp(0, pid);
    kill(pid, SIGCONT);
    fgpid = pid;
    fgrun = 1;
    while(fgrun==1) ; /* wait until foreground job terminates */
    tcsetpgrp(0, getpid());
    fgpid = getpid();
}

void bg(int job_id) {
    strcpy(bg_jobs[job_id].state, "Running");
    job bg = bg_jobs[job_id];
    int isCur = 0;
    int isPrev = 0;
    if(bg.pid==curbgpid) isCur = 1;
    else if (bg.pid==prevbgpid) isPrev = 1;
    pid_t pid = bg_jobs[job_id].pid;
    printf("[%d]%s%s\t%s &\n", bg.job_id, isCur ? "+" : "" , isPrev ? "-" : "" , bg.command);
    kill(pid, SIGCONT);
    sleep(0); /* wait a little bit before return to prompt */
}

int findCurJobId() {
    for(int i = 1; i<MAXJOBS; i++) {
        if (bg_jobs[i].pid == curbgpid) {
            return bg_jobs[i].job_id;
        }
    }
    return 0;
}

/*
    create process / execute a program / wait for it to complete.
*/
void forkExec(char **input, char *file, int background, char *cmd) {

    sigset_t mask;
    pid_t pid = fork();
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL); /* block SIGCHLD to avoid race condition with add and delete job */

    if (pid < 0) printf("fork() failed\n");
    else if (pid == 0) { /* child process */
        enableDefaultHandlers(); /* restore default sigaction and unbliock SIGCHLD */
        sigprocmask(SIG_UNBLOCK, &mask,NULL);
        ioRedirection(file);
        pid = getpid();
        setpgid(pid, pid);
        if(!background) {
            tcsetpgrp(0, pid);
            fgpid = pid;
        }
        execvp(input[0], input); /* run the external command. */
        printf("bad command\n");
        exit(0);
    }
    else {
        setpgid(pid, pid);
        int jobID = findFreeJobID();
        if(background) {
            addBgJob(pid, jobID, cmd, "Running");
            pid_t temp = curbgpid;
            curbgpid = pid;
            prevbgpid = temp;
            sigprocmask(SIG_UNBLOCK, &mask,NULL);
            printf("[%d] %d\n", jobID, pid);
        }
        else {
            updateFg(foreground, pid, jobID, cmd, "foreground");
            sigprocmask(SIG_UNBLOCK, &mask,NULL);
            tcsetpgrp(0, pid);
            fgpid = pid;
            fgrun = 1;
            while(fgrun==1) ; /* wait until foreground job terminates */
            tcsetpgrp(0, getpid());
            fgpid = getpid();
        }
    }
}

void printEcho(char **ptr) {

    if((ptr[1]!=NULL) && strcmp(ptr[1], "$?")==0 && ptr[2]==NULL) {
        printf("%d\n", exitCode);
    }
    else {
        int i = 1;
        while(ptr[i]!=NULL) {
            printf("%s ", ptr[i]);
            i++;
        }
        printf("\n");
        exitCode = 0;
    }
}

void changeDir(char **ptr) {
    if(ptr[1]==NULL || strcmp(ptr[1],"~")==0) {
        chdir(getenv("HOME"));
    }
    else {
        if(chdir(ptr[1])==-1) {
            printf("cd: no such file or directory: %s\n", ptr[1]);
        }
    }
}

/* 
check if there is I/O redirection and if it is background process
return whether it is background process
*/
int check(char line[]) {
    isIn = 0;
    isOut = 0;
    int bg = 0;

    if(strchr(line, '<')!=NULL) {
        isIn = 1;
    }
    else if (strchr(line, '>')!=NULL) {
        isOut = 1;
    }

    if (strchr(line, '&')!=NULL) {
        bg = 1;
    }
    return bg;
}

/*
    run the input command.
*/
void commandHelper(char line[]) {

    int background = check(line);
    /* separate the command and the file name. */
    char *token;
    token = strtok(line, "<>&\n");
    char *input = token;
    char copyInput[1000];
    strcpy(copyInput, input);

    token = strtok(NULL, " \n");
    char *file = token;
    /* initialize ptr and put words in input into ptr. */
    char *ptr[50] = {0};
    makeDoublePointer(ptr, input);

    /* if the command is echo */
    if(strcmp(ptr[0], "echo") == 0) {
        ioRedirection(file);
        printEcho(ptr);
    }
    /* if the command is exit */
    else if (strcmp(ptr[0], "exit") == 0) {
        if(ptr[1]!=NULL) {
		    printf("bye\n");
        	int status = atoi(ptr[1]);
        	status = status & 0xFF;
        	exit(status);
	    }
	    printf("no exit code\n");
    }
    else if (strcmp(ptr[0], "jobs")==0) {
        printJobsList();
    }
    else if (strcmp(ptr[0], "fg")==0) {
        if((ptr[1]!=NULL) && (strncmp(ptr[1], "%%", 1)==0)) {
            char *jobID = strtok(ptr[1], "%%");
            int job_id = atoi(jobID);
            if(bg_jobs[job_id].job_id==0) { 
                printf("fg: %%%d: no such job\n", job_id);
            }
            else { 
                fg(job_id);
            }
        }
        else if (ptr[1]==NULL) {
            int job_id = findCurJobId();
            if(job_id!=0) {
                fg(job_id);
            }
            else {
                printf("fg: no current job\n");
            }
        }
    }
    else if (strcmp(ptr[0], "bg")==0) {
        if((ptr[1]!=NULL) && (strncmp(ptr[1], "%%", 1)==0)) {
            char *jobID = strtok(ptr[1], "%%");
            int job_id = atoi(jobID);
            if(bg_jobs[job_id].job_id==0) { 
                printf("bg: %%%d: no such job\n", job_id);
            }
            else { 
                bg(job_id);
            }
        }
        else if (ptr[1]== NULL){
            int job_id = findCurJobId();
            if(job_id!=0) {
                bg(job_id);
            }
            else {
                printf("bg: no current job\n");
            }
        }
    }
    else if(strcmp(ptr[0], "cd")==0) {
        changeDir(ptr);
    }
    /* else, run the external command. */
    else { 
        forkExec(ptr, file, background, copyInput);
    }
}

int command(char input[], char previousInput[], int hasPreviousCmd) {
    char line[MAXLINE];
    strcpy(line, input);

    const char s[2] = " ";
    char *cmd = strtok(input, s);

    /* if it is double bang, run the previous command. */
    if (strncmp(cmd, "!!", 2) == 0) {
        if(hasPreviousCmd == 1) {
            char previousLine[MAXLINE];
            strcpy(previousLine, previousInput);
            printf("%s", previousLine);
            commandHelper(previousLine);
        }
    }
    /* otherwise, run the command if it is not whitespace or newline. */
    else if (strcmp(cmd, "\n")!=0) {
        strcpy(previousInput, line);
        hasPreviousCmd = 1;
        commandHelper(line);
    }
    return hasPreviousCmd;
}

int main(int argc, char *argv[]) {

    char input[MAXLINE];
    char previousLine[MAXLINE];
    int hasPreviousCmd = 0;

    /* set the signal handlers in sig.c */
    enableSignalHandlers();
    foreground.pid = 0;
    foreground.job_id = 0;
    foreground.command = malloc(1000*sizeof(char));
    foreground.state = malloc(10*sizeof(char));


    if(argc<2) { /* when there is no file input, take user input. */
        printf("Starting IC shell\n");
        while(1) {
            printf("icsh $ ");
            fflush(stdout);
            if ((fgets(input, sizeof(input), stdin))==NULL && ferror(stdin)) {
                //perror("error");
                continue;
            }
            else {
                hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
            }
        }
    }
    else { /* read commands from given file, one by one. */
        FILE* fp;
        for (int i=1; i<argc; i++) {
            fp = fopen(argv[i], "r");
            while(fgets(input, MAXLINE, fp)) {
                if(ferror(fp)) {
                    //perror("error");
                    continue;
                }
                else {
                    hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
                }
            }
            fclose(fp);
        }
    }
    free(foreground.command);
    free(foreground.state);
    return 0;
}