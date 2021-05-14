#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
void forkExec(char **input) {
    pid_t pid = fork();
    if (pid < 0) printf("fork() failed\n");
    else if (pid == 0) {
        // child process
        execvp(input[0], input); // run the external command.
        printf("bad command\n");
        exit(0);
    }
    else {
        int status;
        pid_t pid = wait(&status); // wait for a child process to finish.
    }
}

/*
    run the input command.
*/
void commandHelper(char input[]) {

    // initialize ptr and put words in input into ptr.
    char *ptr[50] = {0};
    makeDoublePointer(ptr, input);

    // if the command is echo
    if(strcmp(ptr[0], "echo") == 0) {
        int i = 1;
        while(ptr[i]!=NULL) {
            printf("%s ", ptr[i]);
            i++;
        }
        printf("\n");
    }

    // if the command is exit
    else if (strcmp(ptr[0], "exit") == 0) {
        printf("bye\n");
        int status = atoi(ptr[1]);
        status = status & 0xFF;
        exit(status);
    }

    // else, run the external command.
    else { 
        forkExec(ptr);
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
