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

/* 
Reference: 
sigchld.c, sig_block.c
https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
https://stackoverflow.com/questions/32708086/ignoring-signals-in-parent-process
https://stackoverflow.com/questions/6335730/zombie-process-cant-be-killed
https://stackoverflow.com/questions/11515399/implementing-shell-in-c-and-need-help-handling-input-output-redirection
*/

int exitCode = 0;
int isIn;
int isOut;

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

/*
    create process / execute a program / wait for it to complete.
*/
void forkExec(char **input, char *file) {

    pid_t pid = fork();

    if (pid < 0) printf("fork() failed\n");
    // child process
    else if (pid == 0) {

        // restore default sigaction
        enableDefaultHandlers();

        ioRedirection(file);

        pid = getpid();
        setpgid(pid, pid);
        tcsetpgrp(0, pid);
        // run the external command.
        execvp(input[0], input);
        printf("bad command\n");
        exit(0);
    }
    else {
        setpgid(pid, pid);
        tcsetpgrp(0, pid);
        int status;
        waitpid(pid, &status, WUNTRACED); // wait for a child process to stop.
        tcsetpgrp(0, getpid());
    
        if (WIFEXITED(status)) { 
            exitCode = WEXITSTATUS(status);
        }
        // print new line if the child process terminate abnormally
        else { printf("\n"); }
        
        if(WIFSIGNALED(status)) {
            exitCode = 128 + WTERMSIG(status);
        }
        if(WIFSTOPPED(status)) {
            exitCode = 128 + WSTOPSIG(status);
        }
    }
}

/*
    run the input command.
*/
void commandHelper(char line[]) {

    isIn = 0;
    isOut = 0;

    if(strchr(line, '<')!=NULL) {
        isIn = 1;
    }
    else if (strchr(line, '>')!=NULL) {
        isOut = 1;
    }

    // separate the command and the file name.
    char *token;
    token = strtok(line, "<>");
    char *input = token;

    token = strtok(NULL, " \n");
    char *file = token;

    // initialize ptr and put words in input into ptr.
    char *ptr[50] = {0};
    makeDoublePointer(ptr, input);

    // if the command is echo
    if(strcmp(ptr[0], "echo") == 0) {
        ioRedirection(file);
        if(strcmp(ptr[1], "$?")==0 && ptr[2]==NULL) {
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

    // if the command is exit
    else if (strcmp(ptr[0], "exit") == 0) {
        if(ptr[1]!=NULL) {
		    printf("bye\n");
        	int status = atoi(ptr[1]);
        	status = status & 0xFF;
        	exit(status);
	    }
	    printf("no exit code\n");
    }

    // else, run the external command.
    else { 
        forkExec(ptr, file);
    }
}

int command(char input[], char previousInput[], int hasPreviousCmd) {
    char line[1000];
    strcpy(line, input);

    const char s[2] = " ";
    char *cmd = strtok(input, s);

    // if it is double bang, run the previous command.
    if (strncmp(cmd, "!!", 2) == 0) {
        if(hasPreviousCmd == 1) {
            char previousLine[1000];
            strcpy(previousLine, previousInput);
            printf("%s", previousLine);
            commandHelper(previousLine);
        }
    }
    // otherwise, run the command if it is not whitespace or newline.
    else if (strcmp(cmd, "\n")!=0) {
        strcpy(previousInput, line);
        hasPreviousCmd = 1;
        commandHelper(line);
    }
    return hasPreviousCmd;
}

int main(int argc, char *argv[]) {

    char input[1000];
    char previousLine[1000];
    int hasPreviousCmd = 0;

    // set the signal handlers in sig.c
    enableSignalHandlers();

    if(argc<2) { // when there is no file input, take user input.
        printf("Starting IC shell\n");
        while(1) {
            printf("icsh $ ");
            fgets(input, sizeof(input), stdin);
            hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
        }
    }
    else { // read commands from given file, one by one.
        FILE* fp;
        for (int i=1; i<argc; i++) {
            fp = fopen(argv[i], "r");
            while(fgets(input, 1000, fp)) {
                hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
            }
            fclose(fp);
        }
    }
    return 0;
}