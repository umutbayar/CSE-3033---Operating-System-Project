#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>
//#include <jmorecfg.h>


#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

typedef struct child{
    pid_t pid;
    struct child *next;
}child;

typedef struct alias_node{
    char alias[80];
    char command[80];

    struct alias_node *next;
}alias_node;

void run_command(int background, char *args[], char *envPath);
void execute(int background, char *args[], char *envPath);
void setup(char inputBuffer[], char *args[],int *background);
void alias(char *args[]);
void unalias(char *args[]);
int search_run_alias(int background, char *args[]);
void alias_list();
void fg();
int redirect(char *args[], int background, char *envPath);
void removeChar(char *str, char garbage);
void clone_args(char *args[], char *args_clone[], int start, int end, int ptr);

pid_t fg_pid;
child * child_head = NULL;
alias_node * alias_head = NULL;

static void signal_handler(int signo){
    if(fg_pid!=0){
        kill(fg_pid, SIGKILL);
    }else{
        printf("There is nothing to kill.");
    }
    fflush(stdout);
    printf("\nmyshell: ");
}

int main(int argc, char *argv[])
{
    char *envPath = getenv("PATH");
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */

    struct sigaction act;
    act.sa_handler = signal_handler;            /* set up signal handler */
    act.sa_flags = SA_RESTART;
    if ((sigemptyset(&act.sa_mask) == -1) ||
        (sigaction(SIGTSTP, &act, NULL) == -1)) {
        fprintf(stdout,"Failed to set SIGTSTP handler!\n");
        return 1;
    }

    while (1){
        background = 0;
        printf("myshell: ");
        /*setup() calls exit() when Control-D is entered */
        fflush(NULL);
        setup(inputBuffer, args, &background);

        if(args[0] == NULL) /*if user enter without anything*/
        {
            continue;
        }
        run_command(background, args, envPath);

        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
}

void execute(int background, char *args[], char *envPath){

    if(search_run_alias(background, args))
        return;

    pid_t pid = fork();

    if (pid<0)
        fprintf(stderr, "Fork failed!\n");


    if (pid == 0){

        char *token = strtok(envPath, ":");
        char temp[80] = "";
        strcpy(temp, token);
        strcat(temp, "/");
        strcat(temp, args[0]);
        while(execl(temp, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], NULL) != 0){

            token = strtok(NULL, ":");
            strcpy(temp ,token);
            strcat(temp, "/");
            strcat(temp, args[0]);
        }
        printf("asfgasdfasdf\n");
        fprintf(stdout, "Command %s not found\n", args[0]);
        exit(0);
    }

    if (background != 0){
        if(child_head == NULL){
            child_head = (child*)malloc(sizeof(child));
            child_head->pid = pid;
            child_head->next = NULL;
        }else{
            child *new_child;
            new_child = (child*)malloc(sizeof(child));
            new_child->pid = pid;
            new_child->next = child_head;
            child_head = new_child;
        }
        printf("[%d] : %d \n", background, pid);
    }else{
        fg_pid = pid;
        waitpid(pid,NULL,0);
        fg_pid = 0;
    }

}

void run_command(int background, char *args[], char *envPath){

    if (strcmp(args[0], "alias") == 0){
        alias(args);
        return;
    }else if (strcmp(args[0], "exit") == 0){
        printf("Bye!");
        exit(0);
    } else if (redirect(args, background, envPath)){
        return;
    } else if (strcmp(args[0], "fg") == 0){
        fg();
        return;
    } else if (strcmp(args[0], "clr") == 0){
        system("clear");
    } else if (strcmp(args[0], "unalias") == 0){
        unalias(args);
        return;
    }else{
        execute(background, args, envPath);
        return;
    }
}

void fg(){
    if(child_head != NULL){
        pid_t pid = child_head->pid;

        if(!tcsetpgrp(getpid(), getpgid(pid))){
            fprintf(stdout, "Error!");
        }else{
            fg_pid = pid;
            waitpid(pid,NULL,0);
            fg_pid = 0;
        }
        child * temp = child_head;
        child_head = child_head->next;
        //free(temp);
    } else{
        fprintf(stderr, "There is no background process!\n");
        return;
    }
}
void alias(char *args[]){
    int i= 0;
    char command[80] = "";
    int flag = 0;

    if(strcmp(args[1], "-l") == 0){
        alias_list();
        return;
    }

    while(args[i]!= NULL)
    {
        if(args[i][0] == '\"')
        {
            if(args[i][strlen(args[i])-1] == '\"'){
                flag = 1;
            }
            removeChar(args[i],'\"');
            strcat(command,args[i]);
            break;
        }
        i++;
    }

    i++;

    while(args[i]!= NULL && flag == 0)
    {
        if(args[i][strlen(args[i])-1] == '\"')
        {
            strcat(command," ");
            removeChar(args[i],'\"');
            strcat(command,args[i]);
            break;
        }
        strcat(command," ");
        strcat(command,args[i]);
        i++;
    }

    if(args[i+1] == NULL){
        fprintf(stdout, "Key word is missing!\n");
        return;
    }

    if(alias_head == NULL){
        alias_head= (alias_node*)malloc(sizeof(struct alias_node));
        strcpy(alias_head->command, command);
        strcpy(alias_head->alias, args[i+1]);
        alias_head->next = NULL;
        printf("alias added: %s, %s\n", alias_head->alias, alias_head->command);
    }else{
        alias_node *new_child;
        new_child = (alias_node*)malloc(sizeof(struct alias_node));
        strcpy(new_child->command, command);
        strcpy(new_child->alias, args[i+1]);
        new_child->next = alias_head;
        alias_head = new_child;
        printf("alias added: %s, %s\n", alias_head->alias, alias_head->command);
    }
}

void unalias(char *args[]){
    alias_node * ptr = alias_head;

    while(ptr != NULL){
        if(ptr->next != NULL && (strcmp(args[1],ptr->next->alias) == 0)){
            alias_node * temp = ptr->next;
            ptr->next = ptr->next->next;
            free(temp);
            printf("alias %s is removed.\n", args[1]);
            break;
        }else if (ptr->next == NULL && strcmp(args[1],ptr->alias) == 0){
            free(ptr);
            alias_head = NULL;
            printf("alias %s is removed.\n", args[1]);
            break;
        }
        ptr = ptr->next;
    }

}

int search_run_alias(int background, char *args[]){

    alias_node * ptr = alias_head;

    while(ptr != NULL){
        if(strcmp(args[0],ptr->alias) == 0){
            pid_t pid = fork();

            if(pid<0){
                fprintf(stdout,"Fork failed!\n");
            } else if(pid == 0){
                system(ptr->command); /*run command in the child*/
                exit(0);
            }

            if(background == 0){
                fg_pid = pid;
                waitpid(pid, NULL, 0); /*run in foreground if background == 0*/
                fg_pid = 0;
                return 1;
            }else{
                if(child_head == NULL){
                    child_head = (child*)malloc(sizeof(child));
                    child_head->pid = pid;
                    child_head->next = NULL;
                }else{
                    child *new_child;
                    new_child = (child*)malloc(sizeof(child));
                    new_child->pid = pid;
                    new_child->next = child_head;
                    child_head = new_child;
                }
                printf("[%d] : %d \n", background, pid);
                return 1;
            }
        }
        ptr = ptr->next;
    }
    return 0;
}

void alias_list(){
    alias_node * ptr = alias_head;

    if (ptr == NULL){
        printf("There is nothing to list.\n");
    }
    while (ptr != NULL){
        printf("%s --> %s\n", ptr->alias, ptr->command);
        ptr = ptr->next;
    }
}

int redirect(char *args[], int background, char *envPath) { /*checks for any redirection command and execuate if exists*/


    /*find any redirect arg if exists*/
    char *args_clones[MAX_LINE / 2 + 1];
    int i = 0, ptr1 = 0, ptr2 = 0, flag=0;

    while (args[i] != NULL) {
        if (strcmp("<", args[i]) == 0 || strcmp(">>", args[i]) == 0 ||
            strcmp("2>", args[i]) == 0 || strcmp(">", args[i]) == 0) { /*check if there is any redirection command*/
            flag = 1;
            break;
        }
        i++;
    }

    if (args[i] == NULL){ /*return 0 if there is no redirection command*/
        return 0;
    }

    if(args[i+1] == NULL){
        fprintf(stdout,"Missing agrument.\n");
    }
    pid_t pid = fork();

    if (pid < 0)
        fprintf(stderr, "Fork failed!\n");

    if (pid == 0) {
        if(strcmp(">", args[i]) == 0){
            int fd;
            args[i]=NULL;
            fd = open(args[i+1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

        }else if(strcmp(">>", args[i]) == 0){
            int fd;
            args[i]=NULL;
            fd = open(args[i+1], O_WRONLY | O_APPEND | O_CREAT, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

        }else if(strcmp("2>", args[i]) == 0){
            int fd;
            fd = open(args[i+1], O_WRONLY | O_APPEND | O_CREAT, 0644);
            dup2(fd, STDERR_FILENO);
            close(fd);

        }else if(strcmp("<", args[i]) == 0){
            if(args[i+2]!= NULL && strcmp(">", args[i+2]) == 0){
                fflush(stdout);
                if(args[i+3] == NULL){
                    fprintf(stdout, "Missing argument!\n");
                    return 0;
                }
                int fd, fd2;
                fd = open(args[i+1], O_RDONLY, 0644);
                fd2 = open(args[i+3],O_WRONLY | O_TRUNC | O_CREAT, 0644);
                dup2(fd2, STDOUT_FILENO);
                dup2(fd, STDIN_FILENO);
                close(fd2);
                args[i]=NULL;
            }else{
                int fd;
                args[i]=NULL;
                fd = open(args[i+1], O_RDONLY, 0644);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        }

        if(search_run_alias(background, args))
            return 1;


        char *token = strtok(envPath, ":");
        char temp[80] = "";
        strcpy(temp, token);
        strcat(temp, "/");
        strcat(temp, args[0]);
        while (execl(temp, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
                     args[10], args[11], args[12], NULL) != 0) {

            token = strtok(NULL, ":");
            strcpy(temp, token);
            strcat(temp, "/");
            strcat(temp, args[0]);
        }
        fprintf(stdout, "Command %s not found\n", args[0]);
        exit(0);
    }

    if (background != 0) {
        if (child_head == NULL) {
            child_head = (child *) malloc(sizeof(child));
            child_head->pid = pid;
            child_head->next = NULL;
        } else {
            child *new_child;
            new_child = (child *) malloc(sizeof(child));
            new_child->pid = pid;
            new_child->next = child_head;
            child_head = new_child;
        }
        printf("[%d] : %d \n", background, pid);

    } else {
        fg_pid = pid;
        waitpid(pid,NULL,0);
        fg_pid = 0;
    }

}

void clone_args(char *args[], char *args_clones[], int start, int end, int ptr){

    for(int i=start; i<end; i++){
        args_clones[ptr] = (char*)malloc(strlen(args[i])+1);
        memcpy(args_clones[ptr], args[i], strlen(args[i]));
        printf("cloned arg:%s\n", args_clones[ptr]);
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

    //printf(">>%s<<",inputBuffer);
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
                    i++;
                    args[ct] = NULL;
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

//	for (i = 0; i <= ct; i++)
//		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

void removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}