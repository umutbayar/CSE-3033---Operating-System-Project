/*
    Authors;
        - Fatma BALCI - 150119744
        - Alper DOKAY - 150119746
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

// This is the variable kept to existence of foreground process
int isForegroundProc = 0;
// This is the variable kept to id of foreground process
int foregroundProcId;
// This is the variable kept to access to each variable, will be incremented in setup
int argCount = 0;

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;
    argCount = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }

    printf(">>%s<<", inputBuffer);
    for (i = 0; i < length; i++)
    { /* examine every character in the inputBuffer */

        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

    for (i = 0; i <= ct; i++) {
        argCount = argCount + 1;
        printf("\nargs %d = %s\n", i, args[i]);
    }
        
} /* end of setup routine */

struct alias
{
    char **commandArgs;      // This double pointer is put to keep the value inside quotation marks
    char *name;             // name of the alias
    struct alias *nextAlias; // next item to have this struct as linked list
};

struct BackgroundProcess
{   // this struct is implemented to store background processes as linked list
    char **commandArgs;                           // this field is defined to keep given command and args from the shell
    struct BackgroundProcess *nextBackgroundProc; // this field is put as self-reference object to create a link between objects to have them all
    pid_t backgroundProcId;                       // this field is defined to store background process id in the OS
};

// This is the function written to remove a value in alias linked list. 
void deleteAlias(struct alias *aliasToBeDeleted, struct alias **headAlias)
{                                     
    struct alias* temp = *headAlias;
    struct alias* prev = NULL;

    if (temp != NULL && temp == aliasToBeDeleted)
    {
        *headAlias = temp->nextAlias; // Changed head
        free(temp);            // free old head
        return;
    }
 
    else
    {
        while (temp != NULL && temp != aliasToBeDeleted)
        {
            prev = temp;
            temp = temp->nextAlias;
        }
        if (temp == NULL)
            return;
    
        prev->nextAlias = temp->nextAlias;
    
        // Free memory
        free(temp);
    }
}

int checkFileExistence(char *specifiedPath)
{
    // check if the file exists or we can have access to it
    // F_OK = File existence
    // X_OK = File access permission
    if (access(specifiedPath, F_OK | X_OK) == -1)
        return 0;

    return 1;
}


void onPressCTRL_Z(int sigNo)
{ // this function is the handler when ctrl-Z is being presses
    int status;
    // check if there is any foreground process
    if (isForegroundProc)
    {                
        // check if the foreground processing running
        kill(foregroundProcId, 0);
        if (errno != ESRCH)
        {
            kill(foregroundProcId, SIGSTOP);              // if there is a foreground process, it will initiate a signal
            waitpid(-foregroundProcId, &status, WNOHANG); //checks if any zombie children exits
        }
        else
        {
            fprintf(stderr, "No process running found!");
            printf("\nmyshell:");
            fflush(stdout);
        }
        isForegroundProc = 0;
    }
    else
    {
        printf("\nmyshell:");
        fflush(stdout);
    }
}


// this function is used to copy the command line args to another array
void copyArgs(char **newArgs, char **givenArgs)
{ 
    int i;
    // the condition of for loop is required to stop when it sees a NULL value or & character (this implies to provide if there is a bg process)
    for (i = 0; (givenArgs[i] != NULL) && (*givenArgs[i] != '&'); i++)
    {                            
        // take all the values of src and put into commandArgs 
        newArgs[i] = strdup(givenArgs[i]);
    }
    newArgs[i] = NULL;
}


// this function is used to copy the command line args to another array, this does not get the latest quotation mark
void copyAliasArgsToRun(char **newArgs, char **givenArgs)
{ 
    int i;
    // the condition of for loop is required to stop when it sees a NULL value or & character (this implies to provide if there is a bg process)
    for (i = 0; (givenArgs[i+1] != NULL) && (*givenArgs[i+1] != '&'); i++)
    {                            
        // take all the values of src and put into commandArgs 
        newArgs[i] = strdup(givenArgs[i]);
    }
    newArgs[i] = NULL;
}

// this function is written to handle the values given for aliases by removing the "" marks from command Args
void copyAliasForArgs(char **newArgs, char **givenArgs)
{   
    int i;
    // we do exactly the same thing here as we have done for copyArgs
    for (i = 1; givenArgs[i] != NULL; i++)
    {
        newArgs[i - 1] = strdup(givenArgs[i]);
    }
    // remove the first quotation mark
    strcpy(newArgs[0], newArgs[0] + 1);      
    // remove the second quoation mark
    strncpy(newArgs[i - 2], newArgs[i - 2], strlen(newArgs[i - 2]) - 1);
    // set proper values for a string to be ended
    newArgs[i - 2][strlen(newArgs[i - 2]) - 1] = '\0';
    // set the latest to NULL which was name of the alias
    newArgs[i - 2] = NULL;
}

int main(void)
{
    char inputBuffer[MAX_LINE];   /*buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    int isProgramExecution;
    int isAliasFunction;
    int hasBackgroundProcess;
    struct BackgroundProcess *bgHead = NULL;
    struct alias *aliasHead = NULL;

    // sigaction is created to handle CTRL+Z operation
    struct sigaction sigAction;
    sigAction.sa_handler = onPressCTRL_Z;

    while (1)
    {
        background = 0;
        printf("myshell: ");
        // this is put here so that we can see the myshell in the beginning
        fflush(stdout);
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
						(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */

        if (args[0] == NULL)
        {
            fprintf(stderr, "Please enter a command, try again! \n");
            continue;
        }

        // if user enter exit, then we should check some conditions and then exit
        if (strcmp(args[0], "exit") == 0)
        {
            isProgramExecution = 0;  // set this to zero as this is not a program execution
            isAliasFunction = 0;    // set this to zero as this is not a alias execution
            hasBackgroundProcess = 0;   // set this to zero as this does not have a bg

            struct BackgroundProcess *currentBg = bgHead;

            while (currentBg != NULL)
            {
                // check if there is any process rather than head
                if (waitpid(currentBg->backgroundProcId, NULL, WNOHANG) >= 1)
                {
                    ;
                }
                // check if user can kill and have any processes under parent's processes
                int status = kill(currentBg->backgroundProcId, 0);
                if (status == 0)
                {
                    fprintf(stderr, "You need to terminate all the processes before you exit!\n");
                    hasBackgroundProcess = 1; // set background process status to 1 as it has more than one here
                    break;
                }
                // go to the next item in linked list
                currentBg = currentBg->nextBackgroundProc;
            }

            if (!hasBackgroundProcess)
            { // exit if there are no background processes left
                exit(0);
            }
        }

        isProgramExecution = 1;
        isAliasFunction = 0;

        if (strcmp(args[0], "unalias") == 0)
        {
            isProgramExecution = 0;  // set this to zero as this is not a program execution, alias execution
            struct alias *nextItem = aliasHead; // take the alias linked list head
            while (nextItem != NULL)
            { 
                // iterate each item to find the item instance
                char *name = nextItem->name;
                if (strcmp(args[1], name) == 0)  // once found, do the related job
                {
                    deleteAlias(nextItem, &aliasHead);
                    break;
                }
                // go to the next item
                nextItem = nextItem->nextAlias;
            }
        }

        if (strcmp(args[0], "alias") == 0)
        {
            // put related code here
            isProgramExecution = 0;     // set this to zero as this is not a program execution, alias execution
            if (strcmp(args[1], "-l") == 0)
            {
                
                struct alias *newAliasNode = aliasHead; // take the alias linked list head
                while (newAliasNode != NULL)   
                {                        
                    // iterate each item to display them
                    // and get the commandArgs for aliases
                    char **aliasArgs = newAliasNode->commandArgs;
                    printf("\t%s \"", newAliasNode->name);
                    for (int i = 0; aliasArgs[i] != NULL; i++)
                    {
                        // print args
                        printf("%s ", aliasArgs[i]); 
                        
                        if (aliasArgs[i+1] == NULL) {  // do not write the latest argument at the end which is the name of alias
                            break;
                        }
                    }
                    // go to the new line 
                    printf("\n"); 
                    // go to the next item                
                    newAliasNode = newAliasNode->nextAlias;
                }
            }
            else
            { 
                // this else part mean there is a new addition here to alias list
                struct alias *newAlias = NULL;
                // check if the list is empty
                if (aliasHead == NULL)
                { 
                    // create the instance in memory
                    newAlias = (struct alias *)malloc(sizeof(struct alias));
                    // take its name from arguments and set to the field of the instance
                    char* newName = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                    strcpy(newName, args[argCount-2]);
                    newAlias->name = newName;
                    // copy the command arguments by ignoring quotation marks
                    char **commandArgs = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                    copyAliasForArgs(commandArgs, args);
                    newAlias->commandArgs = commandArgs;
                    newAlias->nextAlias = NULL;
                    // set it as head
                    aliasHead = newAlias;
                }
                else
                { 
                    // if head is not null, this means we have items in the list
                    // we find the latest item to add the newest at the end
                    newAlias = aliasHead;
                    do
                    {
                        if (newAlias->nextAlias == NULL)
                        {
                            break;
                        }
                        
                        newAlias = newAlias->nextAlias;
                    } while (newAlias != NULL);
                    // create a new node, and point the previous node to the newly created one
                    // do exactly what we have done in the if part above with a new instance
                    struct alias *prevAlias = (struct alias *)malloc(sizeof(struct alias));
                    char **commandArgs = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                    copyAliasForArgs(commandArgs, args);
                    prevAlias->commandArgs = commandArgs;
                    prevAlias->nextAlias = NULL;
                    char* newName = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                    strcpy(newName, args[argCount-2]);
                    prevAlias->name = newName;
                    // set it as the latest element
                    newAlias->nextAlias = prevAlias;
                }
            }
        }

        // check if the given command is found in the alias list
        struct alias *searchNode = aliasHead;
        while (searchNode != NULL) {
            char *name = searchNode->name;
            if (strcmp(name, args[0]) == 0){
                char **aliasArgs = searchNode->commandArgs; 
                copyAliasArgsToRun(args, searchNode->commandArgs);
                for (int i = 0; aliasArgs[i] != NULL; i++)
                    {
                        printf("%s ", aliasArgs[i]);
                        
                        if (aliasArgs[i+2] == NULL) { 
                            break;
                        }
                    }
                isProgramExecution = 1;  // set this to 1 as this will be a program execution
                break;
            }
            // go to the next item
            searchNode = searchNode->nextAlias;
        }
        

        if (isProgramExecution)
        {
            char inputFileContent[MAX_LINE / 2 + 1];
            char outputFileContent[MAX_LINE / 2 + 1];
            pid_t childpid;
            int redirection = 0;
            int fd;
			int fd2;


            // try to create a new process for the program execution
            if ((childpid = fork()) == -1)
            {
                fprintf(stderr, "failed to fork!");
                continue;
            }
            else if (childpid == 0)
            {
                // go through the args to check if we have I/O redirection
                for (int i = 0; args[i] != NULL; i++)
                {
                    // check truncation/creation mode
                    if (strcmp(args[i], ">") == 0)
                    { 
                        if (*(args + i + 1) == NULL)
                        { 
                            // if we cannot file a file we should warn the user and then exit
                            fprintf(stderr, "Target file was not specified!\n");
                            exit(1);
                        }
                        // set it to 1 for switch operation below
                        redirection = 1; 
                        args[i] = NULL;
                        // take output file name to a variable
                        strcpy(outputFileContent, *(args + i + 1)); 
                        break;
                    }
                    // check append/creation mode
                    else if (strcmp(args[i], ">>") == 0)
                    {
                        if (*(args + i + 1) == NULL)
                        {
                            fprintf(stderr, "Target file was not specified!\n");
                            exit(1);
                        }
                        // set it to 2 for switch operation below
                        redirection = 2; 
                        args[i] = NULL;
                        // take output file name to a variable
                        strcpy(outputFileContent, *(args + i + 1));
                        break;
                    }
                    // check input mode
                    else if (strcmp(args[i], "<") == 0)
                    {
                        if (*(args + i + 1) == NULL)
                        {
                            fprintf(stderr, "Source file was not specified!\n");
                            exit(1);
                        }
                        args[i] = NULL;
                        // set it to 3 for switch operation below
                        redirection = 3;
                        strcpy(inputFileContent, *(args + i + 1));
                        if (*(args + i + 2) != NULL && strcmp(args[i + 2], ">") == 0)
                        { //there is an output redirection so it is been handling here
                            if (*(args + i + 3) == NULL)
                            {
                                fprintf(stderr, "Target file was not specified!\n");
                                exit(1);
                            }
                            // set it to 4 for switch operation below
                            redirection = 4;
                            strcpy(outputFileContent, *(args + i + 3));
                            break;
                        }
                        // go out from the loop if we don't have any I/O Redirection operation
                        else if (*(args + i + 2) == NULL)
                        {
                            break;
                        }
                    }
                }

                switch (redirection)
                {

                
                    case 1:
                        // create/truncate mode
                        fd = open(outputFileContent, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)); // open the output file
                        if (fd == -1)
                        { // error handling if file could not open
                            fprintf(stderr, "Failed to open the file");
                            exit(1);
                        }

                        if (dup2(fd, STDOUT_FILENO) == -1)
                        { // error handling if dup2 is unsuccessful
                            fprintf(stderr, "Failed to redirect standard output");
                            exit(1);
                        }

                        if (close(fd) == -1)
                        { // error handling when closing the file
                            fprintf(stderr, "Failed to close the file");
                            exit(1);
                        }
                        break;

                    
                    case 2:
                        // create / append mode
                        fd = open(outputFileContent, (O_WRONLY|O_CREAT|O_APPEND), (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)); // open the output file
                        if (fd == -1)
                        { // error handling if file could not open
                            fprintf(stderr, "Failed to open the file");
                            exit(1);
                        }

                        if (dup2(fd, STDOUT_FILENO) == -1)
                        { // error handling if dup2 is unsuccessful
                            fprintf(stderr, "Failed to redirect standard output");
                            exit(1);
                        }

                        if (close(fd) == -1)
                        { // error handling when closing the file
                            fprintf(stderr, "Failed to close the file");
                            exit(1);
                        }
                        break;

                    case 3:
                        // reading mode
                        fd = open(inputFileContent, O_RDONLY); // open the input file
                        if (fd == -1)
                        { // error handling if file could not open
                            fprintf(stderr, "Failed to open the file");
                            exit(1);
                        }

                        if (dup2(fd, STDIN_FILENO) == -1)
                        { // error handling if dup2 is unsuccessful
                            fprintf(stderr, "Failed to redirect standard input");
                            exit(1);
                        }

                        if (close(fd) == -1)
                        { // error handling when closing the file
                            fprintf(stderr, "Failed to close the file");
                            exit(1);
                        }
                        break;

                    case 4:
                        // read and create/truncate mode
                        fd = open(inputFileContent, O_RDONLY); // open the input file
                        fd2 = open(outputFileContent, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));

                        if ((fd == -1) || (fd2 == -1))
                        { // error handling if file could not open
                            fprintf(stderr, "Failed to open the file");
                            exit(1);
                        }

                        if (dup2(fd, STDIN_FILENO) == -1)
                        { // error handling if dup2 is unsuccessful
                            fprintf(stderr, "Failed to redirect standard input");
                            exit(1);
                        }

                        if (dup2(fd2, STDOUT_FILENO) == -1)
                        { // error handling if dup2 is unsuccessful
                            fprintf(stderr, "Failed to redirect standard input");
                            exit(1);
                        }

                        if ((close(fd) == -1) || (close(fd2) == -1))
                        { // error handling when closing the file
                            fprintf(stderr, "Failed to close the file");
                            exit(1);
                        }
                        break;
                        // </case 4>
                }

                char *path = getenv("PATH");

                // check if we can successfully get the root path
                if (path == NULL)
                {
                    fprintf(stderr, "Error while getting $PATH\n");
                    exit(1);
                }

                // try to split the root path and then find the bin that exists
                char *directoryNames = strtok(path, ":");
                while (directoryNames != NULL)
                {                                                                      
                    char *rootPath = (char *)malloc(strlen(args[0]) + strlen(directoryNames) + 1);
                    strcpy(rootPath, directoryNames);           
                    // append /                                   
                    strcat(rootPath, "/");   
                    // append the given arg or program name                                           
                    strcat(rootPath, args[0]);                                          

                    // check if the file exists
                    if (checkFileExistence(rootPath))
                    { 
                        char **arguments = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                        copyArgs(arguments, args);
                        execv(rootPath, arguments); // if so, execute execv() from child process
                        fprintf(stderr, "An error must have happened when running execv()!");
                        exit(1); // terminate the child process if execv() did not work properly
                    }

                    directoryNames = strtok(NULL, ":");
                }
                fprintf(stderr, "Our shell did not recognize your command!\n");
                exit(1);
            }
            else
            { 
                // if the requested command was the first command, which is parent, then do the following
                if (background == 0)
                {
                    foregroundProcId = childpid;
                    isForegroundProc = 1;
                    waitpid(childpid, NULL, 0); // wait for the child until its terminated
                    isForegroundProc = 0;
                }
                // if the given command is a command should be handled in background
                else
                {
                    struct BackgroundProcess *givenBg = NULL;

                    // this is true when the background process list empty
                    if (bgHead == NULL)
                    {                                                                                    
                        givenBg = (struct BackgroundProcess *)malloc(sizeof(struct BackgroundProcess));
                        givenBg->backgroundProcId = childpid;                                           
                        char **bgArgs = malloc(sizeof(char *) * MAX_LINE / 2 + 1);                        
                        copyArgs(bgArgs, args);                                                           
                        givenBg->commandArgs = bgArgs;                                                 
                        givenBg->nextBackgroundProc = NULL;                                                               
                        bgHead = givenBg;                                                               
                    }
                    // it results in else part when there was more than one value in the background process list
                    else
                    {
                        givenBg = bgHead;

                        // iterate over each element to get the latest one
                        do
                        {
                            if (givenBg->nextBackgroundProc == NULL)
                            {
                                break;
                            }
                            givenBg = givenBg->nextBackgroundProc;
                        } while (givenBg != NULL);

                        struct BackgroundProcess *newBackground = NULL;
                        newBackground = (struct BackgroundProcess *)malloc(sizeof(struct BackgroundProcess));
                        newBackground->backgroundProcId = childpid;                        
                        char **bgArgs = malloc(sizeof(char *) * MAX_LINE / 2 + 1);
                        copyArgs(bgArgs, args);                                    
                        newBackground->commandArgs = bgArgs;                               
                        newBackground->nextBackgroundProc = NULL;                       
                        givenBg->nextBackgroundProc = newBackground;
                    }
                }
            }
        }
    }
}