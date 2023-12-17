#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <libgen.h>
#include <dirent.h> 
#include <signal.h>
 

// File read flags
#define READ_FLAGS (O_RDONLY)

// File create flags
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)

// File append flags
#define APPEND_FLAGS (O_WRONLY | O_CREAT | O_APPEND)

// File read permissions
#define READ_MODES (S_IRUSR | S_IRGRP | S_IROTH)

// File create permissions
#define CREATE_MODES (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


struct listProcess{
	
	int processNumber ;
	pid_t pid ;	     // pid
	char progName[50] ; // program name
	struct listProcess *nextPtr ;
	
};
typedef struct listProcess ListProcess ; 
typedef ListProcess *ListProcessPtr;

//This struct is for bookmark commands
struct bookmark{

	char progName[50] ; // program name which added into bookmark
	struct bookmark *nextPtr ;

};
typedef struct bookmark bookmarks ; 
typedef bookmarks *bookmarkPtr ; 

struct history
{
	char inputArgs[90];
	struct history *previousPtr ;
	struct history *nextPtr ;
};
typedef struct history History ;
typedef History *HistoryPtr ;

void setup(char inputBuffer[], char *args[],int *bg);
int formatOutputSymbol(char *arg);
int checkifexecutable(const char *filename);
int findpathof(char *pth, const char *exe);
void insert(ListProcessPtr *sPtr , pid_t pid , char progName[]);
void insertBookmark(bookmarkPtr *bPtr , char progName[]);
void insertHistory(HistoryPtr *head , HistoryPtr *tail, char inputArgs[]);
int isEmpty(ListProcessPtr sPtr);
int isEmptyBookmark(bookmarkPtr bPtr);
void printListBookmark(bookmarkPtr bPtr);
void printHistory(HistoryPtr hPtr);
void deleteStoppedList(ListProcessPtr *currentPtr);
void deleteBookmarkList(char *charindex,bookmarkPtr *currentPtr);
void runBookmarkIndex(char *charindex, bookmarkPtr currentPtr);
int killAllChildProcess(pid_t ppid);
void childSignalHandler(int signum);
void sigtstpHandler();
void parentPart(char *args[], int *bg , pid_t childPid , ListProcessPtr *sPtr);
void inputRedirect();
void outputRedirect();
void childPart(char path[], char *args[]);
void createProcess(char path[], char *args[],int *bg,ListProcessPtr *sPtr);
int startsWith(const char *pre, const char *str);
int endsWith(const char *pre, const char *suffix);
void printBookmarkUsage();
int isInteger(char arg[]);
void bookmarkCommand(char *args[], bookmarkPtr *startPtrBookmark);
void clearLine(char args[],char lineNumber[]);
void printSearchCommand(char *fileName , char *pattern);
void listFilesRecursively(char *basePath,char *pattern);
void searchCommand(char *args[]);
void printUsageOfIO();
int checkIORedirection(char *args[]);
void formatInput(char *args[]);
int main(void);

char inputFileName[20]; 
char outputFileName[20];
char outputRedirectSymbol[3] = {"00"};

int inputRedirectFlag;
int outputRedirectFlag;

int numOfArgs = 0; //
int processNumber = 1 ; //

pid_t parentPid ; // stores the parent pid
pid_t fgProcessPid = 0;

//This checks if list is empty or not
int isEmpty(ListProcessPtr sPtr){return sPtr == NULL;}

// isempty for bookmark struct
int isEmptyBookmark(bookmarkPtr bPtr){return bPtr == NULL;}

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */


void setup(char inputBuffer[], char *args[],int *bg)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,90);  

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
        fprintf(stderr, "%s", "error reading the command\n");
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
		    *bg  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */
	numOfArgs = ct;

} /* end of setup routine */
 

// next 2 functions is looking PATH and find executable input  

/* int checkifexecutable(const char *filename)
 * 
 * Return non-zero if the name is an executable file, and
 * zero if it is not executable, or if it does not exist.
 */


//This function takes the output symbol and turn it to integer value
int formatOutputSymbol(char *arg)
{

	if(strcmp(arg, ">") == 0)
		return 0;
	else if(strcmp(arg, ">>") == 0)
		return 1;
	else if(strcmp(arg, "2>") == 0)
		return 2;
	else if(strcmp(arg, "2>>") == 0)
		return 3;

	return -1;

}

//This function takes a program name and check it if it is executable or not.
int checkifexecutable(const char *filename)
{
     int result;
     struct stat statinfo;
     
     result = stat(filename, &statinfo);
     if (result < 0) return 0;
     if (!S_ISREG(statinfo.st_mode)) return 0;

     if (statinfo.st_uid == geteuid()) return statinfo.st_mode & S_IXUSR;
     if (statinfo.st_gid == getegid()) return statinfo.st_mode & S_IXGRP;
     return statinfo.st_mode & S_IXOTH;
}

int findpathof(char *pth, const char *exe)
{
     char *searchpath;
     char *beg, *end;
     int stop, found;
     int len;

     if (strchr(exe, '/') != NULL) {
	  if (realpath(exe, pth) == NULL) return 0;
	  return  checkifexecutable(pth);
     }

     searchpath = getenv("PATH");
     if (searchpath == NULL) return 0;
     if (strlen(searchpath) <= 0) return 0;

     beg = searchpath;
     stop = 0; found = 0;
     do {
	  end = strchr(beg, ':');
	  if (end == NULL) {
	       stop = 1;
	       strncpy(pth, beg, PATH_MAX);
	       len = strlen(pth);
	  } else {
	       strncpy(pth, beg, end - beg);
	       pth[end - beg] = '\0';
	       len = end - beg;
	  }
	  if (pth[len - 1] != '/') strncat(pth, "/", 2);
	  strncat(pth, exe, PATH_MAX - len);
	  found = checkifexecutable(pth);
	  if (!stop) beg = end + 1;
     } while (!stop && !found);
	  
     return found;
}



//This is insert function for bg process
void insert(ListProcessPtr *sPtr , pid_t pid , char progName[]){
	
	ListProcessPtr newPtr = malloc(sizeof(ListProcess)); // Create Node
	
	if(newPtr != NULL){
		strcpy(newPtr->progName, progName);
		newPtr->processNumber = processNumber ;
		newPtr->pid = pid;
		newPtr->nextPtr = NULL;
		
		ListProcessPtr previousPtr = NULL;
		ListProcessPtr currentPtr = *sPtr;
		
		while(currentPtr != NULL){
			previousPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}
		
		if(previousPtr == NULL){ //insert to the beginning
			newPtr->nextPtr = *sPtr;
			*sPtr = newPtr;
		}else{
			previousPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
			
		}		
	}
	else{
		fprintf(stderr, "%s", "No memory available\n");
	}
		
}

// inserting program into bookmark struct
void insertBookmark(bookmarkPtr *bPtr , char progName[]){

	bookmarkPtr newPtr = malloc(sizeof(bookmarks));

	if(newPtr != NULL){
		strcpy(newPtr->progName, progName);
		newPtr->nextPtr = NULL;

		bookmarkPtr previousPtr = NULL ;
		bookmarkPtr currentPtr = *bPtr ;

		while(currentPtr != NULL){
			previousPtr = currentPtr ;
			currentPtr = currentPtr->nextPtr ;
		}

		if(previousPtr == NULL){ //insert to the beginning
			newPtr->nextPtr = *bPtr;
			*bPtr = newPtr;
		}else{
			previousPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
			
		}		

	}
	else{
		fprintf(stderr, "%s", "No memory available\n");
	}

}

void insertHistory(HistoryPtr *head , HistoryPtr *tail, char inputArgs[]){

	HistoryPtr newPtr = malloc(sizeof(History)); //create node

	if(newPtr != NULL){

		if(*tail == NULL && *head == NULL){
			strcpy(newPtr->inputArgs,inputArgs);
			newPtr->previousPtr = newPtr ;			// when there is one history , nextPtr is itself and previousPtr is itself too
			newPtr->nextPtr = newPtr ;
			*head = newPtr ;							// head and tail is itself too
			*tail = newPtr ;
		}
		else{										// when there is two history
			strcpy(newPtr->inputArgs,inputArgs);
			newPtr->nextPtr=(*head) ;					// newPtr's nextPtr is last headPtr and headPtr will be newPtr .
			newPtr->previousPtr=(*tail) ;				// newPtr's previousPtr is always tail and tail will not change
			(*head)->previousPtr=newPtr ;				// head's previousPtr will newPtr and head will be newPtr after this   
			(*head) = newPtr ;						//-->ex		  <--------------------next---	
			(*tail)->nextPtr = newPtr ;				//             next---->     next---->    |
													//		*firefox*		 *gedit*		 *ls*
		}											//			  |	<-----prev    <-----prev
	}else{											//			   --prev------------------->
		fprintf(stderr, "%s", "No memory available\n");
	}


}


//This function is for printing the content of bookmark list
void printListBookmark(bookmarkPtr bPtr){

	int i=0 ;
	bookmarkPtr tempPtr = bPtr ;
	if (isEmptyBookmark(bPtr)) fprintf(stderr, "%s", "List is empty\n");
	else{
		while(tempPtr->nextPtr != NULL){
			printf("%d %s\n",i,tempPtr->progName);
			i++;
			tempPtr = tempPtr->nextPtr ;
		}
		printf("%d %s\n",i,tempPtr->progName);
	}

}

void printHistory(HistoryPtr hPtr){

	int i=0 ;

	HistoryPtr temp = hPtr ;
		while(temp->nextPtr != NULL && i!=3){
			printf("%d -> %s , next-> %s , prev-> %s\n",i,temp->inputArgs,temp->nextPtr->inputArgs,temp->previousPtr->inputArgs);
			temp = temp->nextPtr ;
			i++;
		}	

}

//This function is for deleting dead processes from bg processes list
void deleteStoppedList(ListProcessPtr *currentPtr){
	
	int status ;
	
	if((*currentPtr)==NULL)
		return ;
		
	if(waitpid((*currentPtr)->pid,&status,WNOHANG)==-1){
		// if the stopped process is the first 
	
		ListProcessPtr tempPtr = *currentPtr ;
		*currentPtr = (*currentPtr)->nextPtr ;
		free(tempPtr) ;
		deleteStoppedList(currentPtr);
	}
	else{
		// if the stopped process is not the first
		 
		ListProcessPtr previousPtr = *currentPtr ;
		ListProcessPtr tempPtr = (*currentPtr)->nextPtr ;
		
		while(tempPtr!=NULL && waitpid(tempPtr->pid,&status,WNOHANG)!=-1){
			previousPtr = tempPtr;
			tempPtr=tempPtr->nextPtr;
		}
		if(tempPtr!=NULL){
			ListProcessPtr delPtr = tempPtr ;
			previousPtr->nextPtr=tempPtr->nextPtr;
			free(delPtr);
			deleteStoppedList(currentPtr);
		}
			
	}
	
}

//This function is for deleting items from bookmark list
void deleteBookmarkList(char *charindex,bookmarkPtr *currentPtr){

	int index = atoi(charindex);

	if(isEmptyBookmark(*currentPtr))
		fprintf(stderr, "%s", "List is empty\n");
	else{

		// delete first item
		if(index==0){
			bookmarkPtr tempPtr = *currentPtr;
			*currentPtr=(*currentPtr)->nextPtr;
			free(tempPtr);
		}
		else{	// delete others
			bookmarkPtr previousPtr = *currentPtr ;
			bookmarkPtr tempPtr = (*currentPtr)->nextPtr ;
			int temp = 1;

			while(temp!=index && tempPtr!=NULL){
				previousPtr = tempPtr ;
				tempPtr = tempPtr->nextPtr ;
				temp++;
			}
			if(tempPtr==NULL)
				fprintf(stderr, "%s", "There is no bookmark with this index.\n");
			else{

				bookmarkPtr delPtr = tempPtr ;
				previousPtr->nextPtr=tempPtr->nextPtr;
				free(delPtr);

			}

		}

	}

}

//This function is for running the corresponding index from bookmark list.
void runBookmarkIndex(char *charindex, bookmarkPtr currentPtr){

	int index = atoi(charindex);
	char *progpath ;

	if(isEmptyBookmark(currentPtr))
		fprintf(stderr, "%s", "List is empty\n");
	else{

		bookmarkPtr tempPtr = currentPtr;
		int j=0 ;
		while(tempPtr!=NULL && j!=index){
			tempPtr=tempPtr->nextPtr;
			j++;
		}
		if (tempPtr==NULL)
		{
			fprintf(stderr, "%s", "There is no bookmark in this index.\n");
		}
		else{

			char exe[90] ;
			strcpy(exe,tempPtr->progName);
			

			int length = strlen(exe);
			int i = 0;
			exe[length - 2] = '\0';
			for(i = 0 ; i < length; i++){
				exe[i] = exe[i+1];
			}

			char command[100];
			sprintf(command, "%s",exe);
			system(command);

		}

	}

}

//This function is for killing all the sub childs of our foreground process
int killAllChildProcess(pid_t ppid)
{
   char *buff = NULL;
   size_t len = 255;
   char command[256] = {0};

   sprintf(command,"ps -ef|awk '$3==%u {print $2}'",ppid);
   FILE *fp = (FILE*)popen(command,"r");
   while(getline(&buff,&len,fp) >= 0)
   {
 	 killAllChildProcess(atoi(buff));
 	 char cmd[256] = {0};
 	 sprintf(cmd,"kill -TSTP %d",atoi(buff));
 	 system(cmd);
   }
   free(buff);
   fclose(fp);
   return 0;
}

//This function is called when the child is created
void childSignalHandler(int signum) {
	int status;
	pid_t pid;
	
	
	pid = waitpid(-1, &status, WNOHANG);
}


void sigtstpHandler(){ //When we press ^Z, this method will be invoked automatically

	if(fgProcessPid == 0 || waitpid(fgProcessPid,NULL,WNOHANG) == -1){
		printf("\nmyshell: ");
		fflush(stdout);
		return;
	}


	killAllChildProcess(fgProcessPid);

	char cmd[256] = {0};
	sprintf(cmd,"kill -TSTP %d",fgProcessPid); //This is for stopping process
	system(cmd);

	fgProcessPid = 0;
}


//This is for parent part of creating new process 
void parentPart(char *args[], int *bg , pid_t childPid , ListProcessPtr *sPtr){

	if(*bg == 1){ //bg Process

		waitpid(childPid, NULL, WNOHANG);
		setpgid(childPid, childPid); // This will put that process into its process group
		insert(&(*sPtr),childPid,args[0]);
		processNumber++;

	}else{ // Foreground Process

		setpgid(childPid, childPid); // This will put that process into its process group
		fgProcessPid = childPid;

		 if(childPid != waitpid(childPid, NULL, WUNTRACED))
       		fprintf(stderr, "%s", "Parent failed while waiting the child due to a signal or error!!!\n");

	}

}

//This is for input redirection . 
void inputRedirect(){

	int fdInput;

	if(inputRedirectFlag == 1){///This is for getting input from file
		fdInput = open(inputFileName, READ_FLAGS, READ_MODES);

		if(fdInput == -1){
	        fprintf(stderr, "%s", "Failed to open the file given as input...\n");

	        return;
	    }

	    if(dup2(fdInput, STDIN_FILENO) == -1){
	        fprintf(stderr, "%s", "Failed to redirect standard input...\n");
	        return;
	    }

	    if(close(fdInput) == -1){
	        fprintf(stderr, "%s", "Failed to close the input file...\n");
	        return;
	    }

	}

}

//This is for output redirection 
void outputRedirect(){

	int fdOutput;

			// > is 0 | >> is 1 | 2> is 2 | 2>> is 3
		int outputMode = formatOutputSymbol(outputRedirectSymbol);

		if(outputMode == 0){ // For > part

			fdOutput = open(outputFileName, CREATE_FLAGS, CREATE_MODES);

	        if(fdOutput == -1){ 
         	    fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
         	   return;
        	}


        	if(dup2(fdOutput, STDOUT_FILENO) == -1){
				fprintf(stderr, "%s", "Failed to redirect standard output...\n");
				return;
			}

		}
		else if(outputMode == 1){ // for >> part

			fdOutput = open(outputFileName, APPEND_FLAGS, CREATE_MODES);

	        if(fdOutput == -1){ ///
         	   fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
         	   return;
        	}


        	if(dup2(fdOutput, STDOUT_FILENO) == -1){
				fprintf(stderr, "%s", "Failed to redirect standard output...\n");
				return;
			}

		}
		else if(outputMode == 2){	// for 2> part
			fdOutput = open(outputFileName, CREATE_FLAGS, CREATE_MODES);
			if(fdOutput == -1){
				fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
				return;
     	   }

     	   if(dup2(fdOutput, STDERR_FILENO) == -1){
				fprintf(stderr, "%s", "Failed to redirect standard error...\n");
				return;
			}

		}
		else if(outputMode == 3){ // for 2>> part

			fdOutput = open(outputFileName,APPEND_FLAGS, CREATE_MODES);

			if(fdOutput == -1){
				fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
				return;
     	   }

     	   if(dup2(fdOutput, STDERR_FILENO) == -1){
				fprintf(stderr, "%s", "Failed to redirect standard error...\n");
				return;
			}
		}

}

//This is for child
void childPart(char path[], char *args[]){


	if(inputRedirectFlag == 1){ //This is for myshell: myprog [args] < file.in
		inputRedirect();

		if(outputRedirectFlag == 1){ // This is for myprog [args] < file.in > file.out
			outputRedirect();
		}

	}else if(outputRedirectFlag == 1){ // This is for myprog [args] > file.out and myshell: myprog [args] >> file.out and myshell: myprog [args] 2> file.out
		outputRedirect();
	}

	execv(path,args);


}

//This is for creating new child by using fork()
void createProcess(char path[], char *args[],int *bg,ListProcessPtr *sPtr){

	pid_t childPid ;

	childPid = fork();


	if(childPid == -1){fprintf(stderr, "%s", "fork() function is failed!\n"); return;}
	else if(childPid != 0){ // Parent Part
		parentPart(args , &(*bg),childPid , &(*sPtr));

	}else{	//Child Part
		childPart(path , args);

	}


}

//This is for checking the first char of string
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;

}


/*
*	Return if parameter pre ends with suffix or not
*/
int endsWith(const char *pre, const char *suffix){
    if (!pre || !suffix)
        return 0;
    size_t lenstr = strlen(pre);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(pre + lenstr - lensuffix, suffix, lensuffix) == 0;

}

//This is for printing usage of bookmark
void printBookmarkUsage(){
	printf("*Bookmark Usage : \n->\"bookmark -l\" to see the bookmark list.\n->\"bookmark -i (index)\" to run the bookmark index.\n->\"bookmark -d (index)\" to delete the item from bookmark.\n");
}

int isInteger(char arg[]){
	int length , i;
	length = strlen (arg);
    for (i=0;i<length; i++)
        if (!isdigit(arg[i]))
        {
            fprintf(stderr, "%s", "Please check your arguments !\n");
            printBookmarkUsage();
            return 1;
        }
	return 0;
}

//This is for "bookmark" command part
void bookmarkCommand(char *args[], bookmarkPtr *startPtrBookmark){


	int i=0; 
	while(args[i] != NULL){
		i++;
	}

	if(i==1){	
		fprintf(stderr, "Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");		
		return;
	}
	else if((strcmp(args[1],"-h")==0) && i==2){ 
		printBookmarkUsage();
		return;
	}		
	else if((strcmp(args[1],"-l")==0) && i==2){

		printListBookmark(*startPtrBookmark);

	}
	else if((strcmp(args[1],"-i")==0) && i==3){

		if(isInteger(args[2]) == 0){
			runBookmarkIndex(args[2],*startPtrBookmark);
			return;
		}else{
			return;
		}	

	}
	else if((strcmp(args[1],"-d")==0) && i==3){

		if(isInteger(args[2]) == 0){
			deleteBookmarkList(args[2],startPtrBookmark);
			return;
		}	
		else{
			return;
		}

	}
	else if(startsWith("\"",args[1])){

		//This is for checking the last char of command
		int length = strlen(args[numOfArgs -1]);
		char command[100];
		strcpy(command,args[numOfArgs-1]);

		if(command[length -1] != '\"'){
			fprintf(stderr, "%s", "Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
			return;
		}

		//This part is for checking the command. If it is not an executable, then just return.

		char *exec;
		char path[PATH_MAX+1];
		char firstArgument[50];
		int lengthOfFirstArgument = strlen(args[numOfArgs -1]);

		strcpy(firstArgument,args[1]);
		int t = 0;

		if(firstArgument[0] == '\"' && firstArgument[lengthOfFirstArgument - 1] == '\"'){ //for example "ls"

			firstArgument[lengthOfFirstArgument - 1] = '\0'; //to delete last quote

			for(t = 0; t < lengthOfFirstArgument - 1; t++){
				firstArgument[t] = firstArgument[t+1];
			}

		}else if(firstArgument[0] == '\"'){ // for example "ls 

			for(t = 0; t < lengthOfFirstArgument - 1; t++){
				firstArgument[t] = firstArgument[t+1];
			}
			firstArgument[lengthOfFirstArgument-1] = '\0';

		}

		exec = firstArgument;

		if(!findpathof(path,exec)){ // If command which is want to be stored is not existing before, then just return
			fprintf(stderr, "%s", "There is not such a command to store !\n");
			return;
		}

		char exe[90] = {""};
		for(t = 1; t < numOfArgs ; t++){
			strcat(exe,args[t]);
			strcat(exe," ");
		}
		insertBookmark(startPtrBookmark,exe);
		exe[0] = '\0';

	}
	else{
		fprintf(stderr, "%s", "Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
		return;
	}

}

//This is for fixing line and getting the line number.
void clearLine(char args[],char lineNumber[]){
	int i = 0;

	int length = strlen(args);
	int digitNum = 1;

	for(i = 0 ; i < length; i++){
		if(isdigit(args[i])){ //This will determine the number of digits
			lineNumber[i] = args[i];
			digitNum++;
		}
		else break;
	}

	for(i = 0; i < length; i++){
		args[i] = args[digitNum + i];
	}

}

/**
This function takes fileName and pattern arguments. 
Determines the file name , line number and line by using grep.
Then combines all of them and print.

*/
void printSearchCommand(char *fileName , char *pattern){

	char file[1000] = {0};

	char *buff = NULL;
	char fName[256] = {0};
	size_t len = 255;

	sprintf(fName,"grep -rnwl  %s -e %s | awk '{print $0}'",fileName, pattern); //This returns the name of the file
	FILE *fp = (FILE*)popen(fName,"r");

	while(getline(&buff,&len,fp) >= 0)
	{
		strcpy(file,buff);
	}
	free(buff);
	fclose(fp);

	char tempFileName[1000];
	strcpy(tempFileName,file);

	char allLine[256] = {0};

	char result[1024];

	char *buff2 = NULL;
	char command[256] = {0};
	size_t len2 = 256;

	sprintf(command,"grep -rnw  %s -e %s | awk '{print $0}'",fileName , pattern); //This returns the whole line including our pattern

	FILE *fp2 = (FILE*)popen(command,"r");
	while(fgets(result , sizeof(result) , fp2))
	{
		strcpy(allLine,result);

		char lineNumber[15] = {0};

		clearLine(allLine,lineNumber);

		if(strlen(file) < 1  || !isdigit(lineNumber[0])){

		}else{
			file[strlen(file)-1] = '\0';

		//	fprintf(stdout,"%s : %s -> %s",lineNumber,file , allLine);
			printf("%s : %s -> %s",lineNumber,file , allLine);
			strcpy(file,tempFileName);
		}
	}

	free(buff2);
	fclose(fp2);



}


/**
 * Lists all files and sub-directories recursively 
 * considering path as base path.
 */

void listFilesRecursively(char *basePath,char *pattern)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir)
        return;
  

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {	

        	char fName[50];
          
       	   strcpy(fName,dp->d_name);


       	   char grepFile[1000];
       	   strcpy(grepFile , basePath);
       	   strcat(grepFile , "/");
       	   strcat(grepFile , dp->d_name);

       	   if(fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
       	   		 fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H')){


       	   	printSearchCommand(grepFile , pattern);

       	   }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            listFilesRecursively(path,pattern);
        }
    }

    closedir(dir);
}

//This function is for "search" command
void searchCommand(char *args[]){
    bool valid=0;
    if(numOfArgs < 2){
		fprintf(stderr, "%s", "Please check your arguments!!\n");
		valid =1;

	}else if(numOfArgs == 2){ //nonrecursive check

		int length = strlen(args[1]);
		char pattern[100];
		strcpy(pattern,args[1]);


		if(!(pattern[0] == '"' && pattern[length - 1] == '"')){
			fprintf(stderr, "%s", "Please check your arguments!! You need to give your pattern between \" \" \n");
			valid= 1;
		}

	}else if(numOfArgs == 3){ //recursive check
		int length = strlen(args[2]);
		char pattern[100];
		strcpy(pattern,args[2]);


		if(!(pattern[0] == '"' && pattern[length - 1] == '"')){
			fprintf(stderr, "%s", "Please check your arguments!! You need to give your pattern between \" \" \n");
			valid= 1;
		}

		if(strcmp(args[1],"-r") != 0){
			fprintf(stderr, "%s", "Please check your arguments!!\n");
			valid= 1;
		}
	}

	if(valid) return;

	int i=0; 
	while(args[i] != NULL){
		i++;
	}
	
	if(i==2){

		char cmd[1000];		

		// without -r option 
		// it will look all the files which ends .c .C .h .H under current directory and find the 'command' input word in this files


	    struct dirent *de;  // Pointer for directory entry 
	  
	    // opendir() returns a pointer of DIR type.  
	    DIR *dr = opendir("."); 
	  
	    if (dr == NULL){  // opendir returns NULL if couldn't open directory 

	        fprintf(stderr, "%s", "Could not open current directory\n"); 

	    } 
	  
	    /*
		*	Look all files under current directory and add find the files which endsWith .c .C .h .H , after finding call each of them and  
		*   look if the files includes the 'pattern' which comes with argument of search commend
	    */
	    while ((de = readdir(dr)) != NULL) {

	    	if(strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){

				char fName[50];

				strcpy(fName,de->d_name);


	    		if(fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
       	   		 fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H')){

	    			printSearchCommand(de->d_name,args[1]);
	    		}

	    	}

	    }
	            
	  	printf("\n");
	    closedir(dr);     
 

	}
	else if(i==3 && strcmp(args[1],"-r")==0){ // recursive part 

	    char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {

		    listFilesRecursively(cwd,args[2]);
		}
		else {
		    fprintf(stderr, "%s", "getcwd() error\n"); 

		}

	}

	else{
		fprintf(stderr, "%s", "2 ways to use this command :\nsearch 'command'\nsearch 'option' 'command'\n"); 

	}

}

//This function is for printing the usage of redirection 
void printUsageOfIO(){
	printf("[1] -> \"myprog [args] > file.out\"\n");
	printf("[2] -> \"myprog [args] >> file.out\"\n");
	printf("[3] -> \"myprog [args] < file.in\"\n");
	printf("[4] -> \"myprog [args] 2> file.out\"\n");
	printf("[5] -> \"myprog [args] < file.in > file.out\"\n");
}

//If input includes IO operation, then this method will return 0. Otherwise 1
//Contents of inputFileName and outputFileName also set in here 
int checkIORedirection(char *args[]){


	//Error handlings
	if(numOfArgs == 2 && strcmp(args[0], "io") == 0  && strcmp(args[1], "-h") == 0){
		printUsageOfIO();
		return 1;
	}

	int a;
	int io;
	for(a = 0; a < numOfArgs; a++){
		if(strcmp(args[a], "<") == 0 || strcmp(args[a], ">") == 0 || strcmp(args[a], ">>") == 0 || strcmp(args[a], "2>") == 0 || strcmp(args[a], "2>>") == 0){
			io = 1;
		}
	}
	if(io == 1){

		if(strcmp(args[0],"bookmark") == 0 || strcmp(args[0],"search") == 0){
			fprintf(stderr, "I/O redirection is not valid for \" %s \" command!!\n",args[0]);
			return 1;
		}



	}else{
		return 0;
	}

	int i;
	//Check arguments
	for(i = 0; i < numOfArgs; i++){
		if(strcmp(args[i], "<") == 0 && numOfArgs > 3  && strcmp(args[i+2],">") == 0){ // myprog [args] < file.in > file.out
			// We need to make sure there is a file_name entered too.
		    if(i+3 >= numOfArgs){
		        fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
		        args[0] = NULL;
		        return 1;
		    }

		    inputRedirectFlag = 1;
		    outputRedirectFlag = 1;
		    strcpy(outputRedirectSymbol, args[i+2]);
		    strcpy(inputFileName , args[i+1]);
		    strcpy(outputFileName, args[i+3]);

		    return 0;

		}else if(strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0 || strcmp(args[i], "2>>") == 0 ){
			//We need to make sure there is a file_name entered too.
            if(i + 1 >= numOfArgs){
                fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
                args[0] = NULL;
                return 1;
            }

            outputRedirectFlag = 1;
            strcpy(outputRedirectSymbol, args[i]);
            strcpy(outputFileName , args[i+1]);

            return 0;

		}else if(strcmp(args[i],"<") == 0){ // myprog [args] < file.in
			if(i + 1 >= numOfArgs){
                 fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
                args[0] = NULL;
                return 1;
            }

            inputRedirectFlag = 1;
            strcpy(inputFileName , args[i+1]);

            return 0;

		}

	}	


}


//This method clears the input from < > 2> 2>> >> . 
void formatInput(char *args[]){

	int i = 0;
	int a;
	int counter;
	int flag = 0;
	for(i = 0; i < numOfArgs; i++){
		if(strcmp(args[i],"<")== 0 || strcmp(args[i],">")== 0 || strcmp(args[i],">>")== 0 || strcmp(args[i],"2>")== 0 || strcmp(args[i],"2>>")== 0){
			args[i] = NULL;
			a = i;
			counter = i+1;
			flag = 1;
			break;
		}
	}

	if(flag == 0) return; //Eger komutun io ile ilgisi yoksa direkt return
	

	for(i = counter; i < numOfArgs; i++){ 
		args[i] = NULL;
	}

	numOfArgs = numOfArgs - (numOfArgs-a); // Update number of arguments


}

 
int main(void){
	
	char inputBuffer[90]; /*buffer to hold command entered */
	int bg; /* equals 1 if a command is followed by '&' */
	char *args[80]; /*command line arguments */
	char path[PATH_MAX+1];
	char *progpath;
	char *exe;

	system("clear"); //This is for clearing terminal at the beginning

	signal(SIGCHLD, childSignalHandler); // childSignalHandler will be invoked when the fork() method is invoked
	signal(SIGTSTP, sigtstpHandler); //This is for handling ^Z

	parentPid = getpid();
	
	ListProcessPtr startPtr = NULL; //starting pointers
	bookmarkPtr startPtrBookmark = NULL;
		HistoryPtr headHistory = NULL ;
	HistoryPtr tailHistory = NULL ;

	while (parentPid==getpid()){
		bg = 0;
		if(isEmpty(startPtr))		processNumber=1;
		printf("myshell: ");
		fflush(0);

		/*setup() calls exit() when Control-D is entered */
		setup(inputBuffer, args, &bg);


		if(args[0] == NULL) continue; // If user just press "enter" , then continue without doing anything

		//insertHistory(&headHistory,&tailHistory,*args);	
		//printHistory(headHistory);
		progpath = strdup(args[0]);
		exe=args[0];

		if(checkIORedirection(args) != 0){ //// Eger ıo yazımında vs hata varsa error verip yeniden input almalı			
			continue;
		}
	


		formatInput(args);

		if(strcmp(args[0],"exit")==0) {
			deleteStoppedList(&startPtr);
			if(isEmpty(startPtr) != 0){
				exit(1);
			}else{
				fprintf(stderr, "%s", "There are processes running in the bg!\n");			
			}
			
		}
		else if(strcmp(args[0],"search")==0){
			searchCommand(args);
			continue;
		}
		else if(strcmp(args[0],"bookmark")==0){
			bookmarkCommand(args,&startPtrBookmark);	
			continue;
		}
		else if(!findpathof(path, exe)){ /*Checks the existence of program*/
			fprintf(stderr, "No executable \"%s\" found\n", exe);	
			free(progpath);
		}else{
					/*If there is a program, then run it*/
			if(*args[numOfArgs-1] == '&')// If last argument is &, delete it
				args[numOfArgs-1] = '\0';						
			createProcess(path,args,&bg,&startPtr);
		}
		
		 	path[0] = '\0';	
			inputFileName[0] = '\0';
			outputFileName[0] = '\0';
			inputRedirectFlag = 0;
			outputRedirectFlag = 0;
	 }	        
}
