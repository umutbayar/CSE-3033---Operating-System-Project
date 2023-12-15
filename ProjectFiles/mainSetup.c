#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>


#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define ALIAS_LIMIT 70
#define MAX_ALIAS_LENGTH 70
#define MAX_COMMAND_LENGTH 125

typedef struct AliasData {
    char alias[MAX_ALIAS_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    int present;
} AliasData;

typedef struct ChildP {
    pid_t pid;
    struct ChildP *next;
} ChildP;

AliasData *aliases = NULL;
int aliasCount = 0;
int aliasCapacity = 0;
void initializeAliases() {
    aliases = (AliasData *)malloc(ALIAS_LIMIT * sizeof(AliasData));
    if (aliases == NULL) {
        perror("Failed to initialize aliases");
        exit(EXIT_FAILURE);
    }
    aliasCapacity = ALIAS_LIMIT;
}

void addAlias(const char *alias, const char *command) {
    if (aliasCount >= aliasCapacity) {
        aliasCapacity *= 2;
        AliasData *temp=(AliasData *)realloc(aliases, aliasCapacity * sizeof(AliasData));
         if(temp == NULL){
            perror("Failed to reallocate memory for aliases");
            exit(EXIT_FAILURE);
         }
         aliases=temp;
    }

    aliases[aliasCount].present = 1;
    strncpy(aliases[aliasCount].alias, alias, MAX_ALIAS_LENGTH - 1);
    aliases[aliasCount].alias[MAX_ALIAS_LENGTH - 1] = '\0';  // Null terminate to be safe
    strncpy(aliases[aliasCount].command, command, MAX_COMMAND_LENGTH - 1);
    aliases[aliasCount].command[MAX_COMMAND_LENGTH - 1] = '\0';  // Null terminate to be safe

    aliasCount++;
}

pid_t fg_pid;

ChildP *child_head = NULL;
static void signal_handler(int signo) {
    if (fg_pid != 0) {
        if (kill(fg_pid, SIGTERM) == -1) {
            perror("Failed to terminate the foreground process");
        } else {
            printf("Foreground process (%d) terminated.\n", fg_pid);
        }
    } else {
        printf("No foreground process to terminate.\n");
    }

    fflush(stdout);
    printf("\nmyshell: ");
}



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

    for (i = 0; i <= ct; i++)
 printf("args %d = %s\n", i, args[i]);
} /* end of setup routine */

void cleanup() {
    free(aliases);
    // İleride başka bellek serbest bırakma işlemleri eklenirse buraya ekleyebilirsiniz.
}

int main(int argc, char *argv[])
{
    char inputBuffer[MAX_LINE];   /*buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    char *envPath = getenv("PATH");

    struct sigaction act;
    act.sa_handler = signal_handler;
    act.sa_flags = SA_RESTART;
    if ((sigemptyset(&act.sa_mask) == -1) ||
        (sigaction(SIGTSTP, &act, NULL) == -1)) {
        fprintf(stdout, "Failed to set SIGTSTP handler!\n");
        return 1;
    }
      initializeAliases();
    while (1)
    {
        background = 0;
        printf("myshell: ");
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);
if(args[0] == NULL) /if user enter without anything/

        {

            continue;

        }

        interpreter(background, args, envPath);
    }

    cleanup();  // free memory


    return 0;
        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
