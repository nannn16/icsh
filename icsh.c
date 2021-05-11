#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void commandHelper(char line[]) {
    const char s[2] = " ";
    char *word = strtok(line, s);
    char *echo = "echo";
    char *ext = "exit";

    if(strcmp(word, echo) == 0) {
        word = strtok(NULL, s);
        while(word != NULL) {
            printf("%s", word);
            word = strtok(NULL, s);
            if(word != NULL) {
                printf(" ");
            }
        }
    }
    else if (strcmp(word, ext) == 0) {
        printf("bye\n");
        word = strtok(NULL, s);
        int status = atoi(word);
        status = status & 0xFF;
        exit(status);
    }
    else { 
        printf("bad command\n");
    }
}

int command(char input[], char previousInput[], int hasPreviousCmd) {
    char line[1000];
    strcpy(line, input);

    const char s[2] = " ";
    char *cmd = strtok(input, s);
    char *doubleBang = "!!";
    char *echo = "echo";

    if (strncmp(cmd, doubleBang, 2) == 0) {
        if(hasPreviousCmd == 1) {
            char previousLine[1000];
            strcpy(previousLine, previousInput);
            printf("%s", previousLine);
            commandHelper(previousLine);
        }
    }
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

    if(argc<2) {
        printf("Starting IC shell\n");
        while(1) {
            printf("icsh $ ");
            fgets(input, sizeof(input), stdin);
            hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
        }
    }
    else {
        FILE* fp;
        for (int i=1; i<argc; i++) {
            fp = fopen(argv[i], "r");
            while(fgets(input, 1000, fp)) {
                hasPreviousCmd = command(input, previousLine, hasPreviousCmd);
            }
        }
    }
    return 0;
}
