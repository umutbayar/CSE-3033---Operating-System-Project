//Haydar Taha Tunç 150119745
//Emir Ege Eren 150119739
//Burak Dursun 150119743

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

struct history{
    char historyLine[MAX_LINE];
    char time[MAX_LINE];
    pid_t pid;
    struct history* next;
};

struct background{
    pid_t pid;
    struct background* next;
};

void pushBackground(struct background** head, pid_t pid){
    struct background* newNode = (struct background*) malloc(sizeof(struct background));
    if (newNode != NULL){
        newNode->pid = pid;
        newNode->next = (*head);
        (*head) = newNode;
    }
}

void printBackground(struct background** head){
    if (head == NULL){
        printf("There is no called process in history list\n");
    } else{
        struct background *temp = (*head);
        while (temp != NULL){
            if (temp != NULL){
                printf("%d\n", temp->pid);
                temp = temp->next;
            }
        }
    }
}

void pushHistory(struct history** head, char historyLineData[], char timeData[], pid_t pidData){
    struct history* newNode = (struct history*) malloc(sizeof(struct history));
    if (newNode != NULL){
        sprintf(newNode->historyLine, "%s", historyLineData);
        sprintf(newNode->time, "%s", timeData);
        newNode->pid = pidData;
        newNode->next = (*head);
        (*head) = newNode;
    }
}

void printHistory(struct history** head, int choose, int num){
    if (head == NULL){
        printf("There is no called process in history list\n");
    } else{
        struct history *temp = (*head);
        int size = 0;
        if (choose == 0){
            while (temp != NULL){
                if (temp != NULL){
                    printf("%d %s\n", size, temp->historyLine);
                    temp = temp->next;
                }
                size++;
                if (size == 10){
                    break;
                }
            }
        } else{
            if (num > 9){
                printf("Wrong number, number = 0 to 9\n");
                return;
            }
            int checkEmpty = 0;
            while (temp != NULL){
                if (size == num){
                    printf("    PID        TIME CMD\n");
                    printf("    %d       %s %s\n", temp->pid, temp->time, temp->historyLine);
                    struct tm *tm;
                    time_t timeT;
                    time(&timeT);
                    tm = localtime(&timeT);
                    char timeArr[MAX_LINE];
                    sprintf(timeArr, "%d:%d:%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
                    pushHistory(head, temp->historyLine, timeArr, temp->pid);
                    checkEmpty = 1;
                }
                temp = temp->next;
                size++;
            }
            if (checkEmpty == 0){
                printf("There is no history at the %d\n", num);
            }
        }
    }
}

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
    i,      /* loop index for accessing inputBuffer array */
    start,  /* index where beginning of next command parameter is */
    ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

} /* end of setup routine */

int main(void) {
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    char PATH[1000];
    char *paths[MAX_LINE];
    char argDestination[MAX_LINE][MAX_LINE];
    struct history *headHistory = NULL;
    struct background *headBackground = NULL;
    int i = 0;
    while (i < MAX_LINE) {
        paths[i] = (char *) malloc(MAX_LINE * sizeof(char));
        i++;
    }
    i = 0;
    memset(PATH, '\0', sizeof(PATH));
    sprintf(PATH, "%s", getenv("PATH"));
    char *pathFile = strtok(PATH, ":");
    while (pathFile != NULL) {
        sprintf(paths[i], "%s", pathFile);
        pathFile = strtok(NULL, ":");
        i++;
    }
    int count = i;
    char buffer[MAX_LINE][MAX_LINE];
    memset(inputBuffer, '\0', sizeof(inputBuffer));
    memset(args, '\0', sizeof(args));
    while (1) {
        background = 0;
        printf("myshell: \n");
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);
        i = 0;
        int argDestinationSize = 0;
        while (args[i] != NULL) {
            char temp[MAX_LINE];
            sprintf(temp, "%s", args[i]);
            sprintf(argDestination[i], "%s", temp);
            i++;
        }
        i = 0;
        while (args[i] != NULL) {
            argDestinationSize++;
            i++;
        }
        if (strncmp(args[0], "history", strlen("history")) == 0) {
            if (args[1] == NULL) {
                printHistory(&headHistory, 0, 0);
            } else if (strncmp(args[1], "-i", strlen("-i")) == 0) {
                printHistory(&headHistory, 1, atoi(args[2]));
            }
        } else if (strncmp(args[0], "fg", strlen("fg")) == 0) {
            printBackground(&headBackground);
        }  else if (strncmp(args[0], "exit", strlen("exit")) == 0) {
            break;
        } else {
            i = 0;
            int argSize = 0;
            while (args[i] != NULL) {
                argSize++;
                i++;
            }
            i = 0;
            char tempBuffer[100];
            while (i < count) {
                sprintf(tempBuffer, "%s/%s", paths[i], args[0]);
                if (access(tempBuffer, X_OK) == 0) {
                    int j = 0;
                    while (args[j] != NULL) {
                        if (j == 0) {
                            sprintf(buffer[j], "%s", tempBuffer);
                        } else {
                            sprintf(buffer[j], "%s", tempBuffer);
                        }
                        j++;
                    }
                    break;
                }
                i++;
            }
            char *newArgs[MAX_LINE];
            i = 0;
            while (i < MAX_LINE) {
                newArgs[i] = (char *) malloc(MAX_LINE * sizeof(char));
                i++;
            }
            i = 0;
            while (args[i] != NULL) {
                if (i == 0) {
                    sprintf(newArgs[i], "%s", buffer[i]);
                } else {
                    sprintf(newArgs[i], "%s", args[i]);
                }
                i++;
            }
            int size = i;
            i = 0;
            while (i < size) {
                sprintf(args[i], "%s", newArgs[i]);
                i++;
            }
            newArgs[i] = NULL;
            pid_t childPid;
            i = 0;
            while (newArgs[i] != NULL) {
                if (strncmp(newArgs[i], "&", strlen("&")) == 0) {
                    newArgs[i] = NULL;
                    break;
                }
                i++;
            }
            struct tm *tm;
            time_t timeT;
            time(&timeT);
            tm = localtime(&timeT);
            if ((childPid = fork()) == -1) {
                printf("%s\n", strerror(errno));
            }
            if (childPid == 0) {
                int a = 0;
                int control = 0;
                char fileName[100];
                int directionRight = 0, directionLeft = 0;
                while (newArgs[a] != NULL){
                    if (strncmp(newArgs[a], ">", strlen(">")) == 0){
                        sprintf(fileName, "%s", newArgs[a+1]);
                        control = 1;
                        break;
                    } else if (strncmp(newArgs[a], ">>", strlen(">>")) == 0){
                        sprintf(fileName, "%s", newArgs[a+1]);
                        control = 2;
                        break;
                    } else if (strncmp(newArgs[a], "<", strlen("<")) == 0){
                        sprintf(fileName, "%s", newArgs[a+1]);
                        control = 3;
                        break;
                    } else if (strncmp(newArgs[a], "2>", strlen("2>")) == 0){
                        printf("Wrong usage %s\n", strerror(errno));
                        exit(1);
                    }
                    a++;
                }
                a = 0;
                char inputFileName[100];
                char outputFileName[100];
                while (newArgs[a] != NULL){
                    if (strncmp(newArgs[a], ">", strlen(">")) == 0){
                        directionRight = 1;
                        sprintf(outputFileName, "%s", newArgs[a+1]);
                    }
                    if (strncmp(newArgs[a], "<", strlen("<")) == 0){
                        directionLeft = 1;
                        sprintf(inputFileName, "%s", newArgs[a+1]);
                    }
                    a++;
                }
                a = 0;
                while (newArgs[a] != NULL){
                    if (strncmp(newArgs[a], ">", strlen(">")) == 0){
                        newArgs[a] = NULL;
                    } else if (strncmp(newArgs[a], ">>", strlen(">>")) == 0){
                        newArgs[a] = NULL;
                    } else if (strncmp(newArgs[a], "<", strlen("<")) == 0){
                        newArgs[a] = NULL;
                    }
                    a++;
                }
                int doubleDirection = 0;
                if (directionRight == 1 && directionLeft == 1){
                    doubleDirection = 1;
                }
                if (doubleDirection == 1){
                    int fdInput;
                    if ((fdInput = open(inputFileName, O_RDONLY)) == -1){
                        printf("%s\n", strerror(errno));
                    }
                    int fdOutput;
                    if ((fdOutput = open(outputFileName, O_WRONLY)) == -1){
                        printf("%s\n", strerror(errno));
                    }
                    if (dup2(fdInput, STDIN_FILENO) == -1){
                        printf("%s\n", strerror(errno));
                    } else if ((close(fdInput) == -1)){
                        printf("%s\n", strerror(errno));
                    }
                    if (dup2(fdOutput, STDOUT_FILENO) == -1){
                        printf("%s\n", strerror(errno));
                    } else if ((close(fdOutput) == -1)){
                        printf("%s\n", strerror(errno));
                    } else {
                        if (execv(newArgs[0], newArgs) == -1) {
                            printf("%s\n", strerror(errno));
                            break;
                        }
                    }
                } else {
                    if (control == 1){
                        int fd;
                        if ((fd = open(fileName, CREATE_FLAGS, CREATE_MODE)) == -1){
                            printf("%s\n", strerror(errno));
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1){
                            printf("%s\n", strerror(errno));
                        } else if ((close(fd) == -1)){
                            printf("%s\n", strerror(errno));
                        } else{
                            if (execv(newArgs[0], newArgs) == -1) {
                                printf("%s\n", strerror(errno));
                                break;
                            }
                        }
                    } else if (control == 2){
                        int fd;
                        if ((fd = open(fileName, O_WRONLY | O_APPEND)) == -1){
                            printf("%s\n", strerror(errno));
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1){
                            printf("%s\n", strerror(errno));
                        } else if ((close(fd) == -1)){
                            printf("%s\n", strerror(errno));
                        } else{
                            if (execv(newArgs[0], newArgs) == -1) {
                                printf("%s\n", strerror(errno));
                                break;
                            }
                        }
                    } else if (control == 3){
                        int fd;
                        if ((fd = open(fileName, O_RDONLY)) == -1){
                            printf("%s\n", strerror(errno));
                        }
                        if (dup2(fd, STDIN_FILENO) == -1){
                            printf("%s\n", strerror(errno));
                        } else if ((close(fd) == -1)){
                            printf("%s\n", strerror(errno));
                        } else{
                            if (execv(newArgs[0], newArgs) == -1) {
                                printf("%s\n", strerror(errno));
                                break;
                            }
                        }
                    } else{
                        if (execv(newArgs[0], newArgs) == -1) {
                            printf("%s\n", strerror(errno));
                            break;
                        }
                    }
                }
            } else {
                i = 0;
                char argBuff[MAX_LINE];
                memset(argBuff, '\0', sizeof(argDestinationSize));
                while (i < argDestinationSize) {
                    char tempBuff[MAX_LINE];
                    char temp[MAX_LINE];
                    sprintf(temp, "%s", argDestination[i]);
                    sprintf(tempBuff, "%s", argBuff);
                    sprintf(argBuff, "%s %s", tempBuff, temp);
                    i++;
                }
                char timeArr[MAX_LINE];
                sprintf(timeArr, "%d:%d:%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
                pushHistory(&headHistory, argBuff, timeArr, childPid);
                if (background == 0) {
                    waitpid(childPid, NULL, 0);
                } else {
                    pushBackground(&headBackground, childPid);
                }
            }
            memset(argDestination, '\0', sizeof(argDestination));
            memset(newArgs, '\0', sizeof(newArgs));
            memset(inputBuffer, '\0', sizeof(inputBuffer));
            memset(args, '\0', sizeof(args));
            memset(buffer, '\0', sizeof(buffer));
            memset(tempBuffer, '\0', sizeof(tempBuffer));
        }
        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
		(3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
}