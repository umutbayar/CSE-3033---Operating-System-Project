#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <termios.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_LINE 80		   /* 80 chars per line, per command, should be enough. */
#define MAX_COMMAND_LEN 25 // sets the max possible length of one single command like ls -l
#define CREATE_APPEND_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_TRUNC_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)
#define READ_FLAGS (O_RDONLY)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int counter = 1;

void tstp_handler(int sig, pid_t foregroundProcess)
{ // ^z Handler 
	signal(SIGTSTP, tstp_handler);
	kill(foregroundProcess, SIGSTOP);
}

/************************************ Datastructures ************************************/

// struct that we will use to hold our background processes
typedef struct process
{
	struct process *next; /* next process in pipeline */
	int isBackground;		 /* background indicator flag */
	pid_t pid;				 /* process ID */
	char completed;			 /* true if process has completed */
	char stopped;			 /* true if process has stopped */
	int status;				 /* reported status value */
	char *args;				 /* the arguments for the process */
	int no;					 /* for ps all displaying */
} process;

process *process_list = NULL;

/*
	Concatenate the arguments for the process

*/
char *concatenate_args(char *args[]){
	int i = 0;
	char *res = malloc(sizeof(char) * 100);
	while (args[i] != NULL)
	{
		char *tmp = malloc(sizeof(char) * sizeof(args[i]));
		memcpy(tmp, args[i], sizeof(args[i]) + 1);
		strcat(res, tmp);
		if (args[i+1] != NULL)
			strcat(res, " ");
		i++;
	}
	return res;
}
/*
	Fuction to display runnig and finished process.
	It goes over the process list and check if the background processes are finished execution or not
	Prints the related process in the related list, running process in running and finished processes in finished
	After displaying the finished process it removes the finished process from list so its not printing it second time 
*/
void ps_all()
{
	process *cursor = process_list;
	while (cursor != NULL)
	{
		if (waitpid(cursor->pid, NULL, WNOHANG)== (cursor->pid)){
			cursor->completed = 1;
		}else{
			cursor->completed = 0;
		}
		cursor = cursor->next;
	}
	
	printf("Running:\n");
	cursor = process_list;
	while (cursor != NULL)
	{
		if (!cursor->completed)
		{
			printf("\t[%d] %s (Pid=%d)\n", cursor->no, cursor->args, cursor->pid);
		}
		cursor = cursor->next;
	}
	cursor = process_list;
	process *tmp;
	printf("Finished:\n");
	while (cursor != NULL)
	{
		if (cursor->completed)
		{
			printf("\t[%d] %s\n", cursor->no, cursor->args);
			if (cursor->next==NULL)
				tmp = NULL;
			else 
				tmp = cursor->next;
			remove_process(cursor);
			cursor = tmp;
			continue;
		}
		cursor = cursor->next;
	}
	if (process_list == NULL){
		counter = 1;
	}
}
/*
	Removes given process from the process list. and clearing memory after removing.
*/
void remove_process( process *remove){
	process *cursor = process_list;
	if (remove == process_list){
		process_list = process_list->next;
		free(remove);
		return;
	}
	while ( cursor->next == remove){
		cursor = cursor->next;
	}
	
	if (remove->next == NULL){
		cursor->next = NULL;
		free(remove);
		return;
	}
	cursor->next = remove->next; 
	free(remove);

}

/*
	Add process to process list. Creates process with its pid, background and args arguments. 
	If there is no process before then it sets as first process of the list. if any goes over it and add to the end
*/
void add_my_process(pid_t pid, int background, char *args)
{
	process *new_process = (process *)malloc(sizeof(process));
	if (new_process == NULL)
	{
		fprintf(stderr, "Error creating newProcess!\n");
		exit(0);
	}
	new_process->pid = pid;
	new_process->no = counter;
	new_process->args = args;
	new_process->isBackground = background;
	new_process->next = NULL;
	counter++;
	if (process_list == NULL)
	{
		process_list = new_process;
		return;
	}
	process *cursor = process_list;
	while (cursor->next != NULL)
	{
		cursor = cursor->next;
	}
	cursor->next = new_process;
}

/*************************************** Bookmark ***************************************/

#pragma region Bookmark

/* Bookmark Struct to keep bookmarks. It has args next and prev attributes. It is designed as doubly linked list structure*/
typedef struct bookmark
{
	char *args;				/*bookmark arguments */
	struct bookmark *next; /* next bookmark */
	struct bookmark *prev; /* previous bookmark*/ 
} bookmark;

bookmark *first_bookmark = NULL;

bookmark *executeBookmark(int pos)
{
	bookmark *cursor = first_bookmark;
	int index = 0;
	while (cursor != NULL)
	{
		if (index == pos)
			break;
		if ((cursor->next == NULL) && index < pos)
		{
			fprintf(stderr, "No such bookmark at that index\n");
			return NULL;
		}
		cursor = cursor->next;
		index++;
	}
	return cursor;
}

/*Prints the bookmark list*/
void printBookmark()
{
	bookmark *cursor = first_bookmark;
	int index = 0, i;
	while (cursor != NULL)
	{
		printf("\t%d \"", index);
		printf("%s", cursor->args);
		printf("\"\n");
		index++;
		cursor = cursor->next;
	}
}
/*Removes the bookmark at the given index.
	Iteratse through the bookmark list and finds the bookmark at that index.
	Removes the related bookmark from list. If there is no bookmark at that index
	just prints erro and informs the user.
*/
void deleteBookmark(int position)
{
	bookmark *cursor = first_bookmark;
	int i = 0;
	if (cursor == NULL)
	{
		fprintf(stderr, "There is no bookmark!!");
		return;
	}
	while (i < position)
	{
		cursor = cursor->next;
		i++;
		if (cursor == NULL)
			break;
	}
	if (cursor == NULL)
	{
		fprintf(stderr, "Sorry, out of range\n");
		return;
	}
	if (cursor == first_bookmark)
	{
		fprintf(stderr, "deleting zero\n");
		if (cursor->next == NULL)
		{
			first_bookmark = NULL;
		}
		else
		{
			cursor = first_bookmark->next;
			cursor->prev = NULL;
			free(first_bookmark);
			first_bookmark = cursor;
		}
		return;
	}
	if (cursor->next == NULL)
	{
		cursor->prev->next = NULL;
		free(cursor);
		return;
	}

	if (i < position)
	{
		fprintf(stderr, "there is no such index\n");
		return;
	}
	cursor->prev->next = cursor->next;
	cursor->next->prev = cursor->prev;
	free(cursor);
	printf("deletion succesfull\n");
}
/*
	Adds given bookmark to the last of bookmark list.
	If there is no bookmark before just sets the bookmark.
	If there is goes over the list and append it to the end of it.
*/
void addBookmark(char *args[])
{
	bookmark *new_bookmark = (bookmark *)malloc(sizeof(bookmark));
	if (new_bookmark == NULL)
	{
		fprintf(stderr, "Error creating bookmark!\n");
		exit(0);
	}
	new_bookmark->args = malloc(sizeof(char) * 100);
	int i = 1;
	char *res = malloc(sizeof(char) * 150);
	while (args[i] != NULL)
	{
		char *tmp = malloc(sizeof(char) * sizeof(args[i]));
		memcpy(tmp, args[i], sizeof(args[i]) + 1);
		strcat(res, tmp);
		if (args[i + 1] != NULL)
			strcat(res, " ");
		i++;
	}
	new_bookmark->args = res;
	new_bookmark->next = NULL;
	new_bookmark->prev = NULL;
	if (first_bookmark == NULL)
	{
		first_bookmark = new_bookmark;
		return;
		fprintf(stderr, "bookmark is null\n");
	}
	bookmark *cursor = first_bookmark;
	while (cursor->next != NULL)
		cursor = cursor->next;
	cursor->next = new_bookmark;
	new_bookmark->prev = cursor;
}

#pragma endregion

/*********************************************************************************************/
/*************************************** Main & Others ***************************************/
/*********************************************************************************************/

void setup(char inputBuffer[], char *args[], int *background)
{
	int length, /* # of characters in the command line */
		i,		/* loop index for accessing inputBuffer array */
		start,	/* index where beginning of next command parameter is */
		ct;		/* index of where to place the next parameter into args[] */

	ct = 0;

	/* read what the user enters on the command line */
	length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

	start = -1;
	if (length == 0)
		exit(0); /* ^d was entered, end of user command stream */

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
		}			 /* end of switch */
	}				 /* end of for */
	args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n", i, args[i]);
} /* end of setup routine */

/*************************************** Search ***************************************/
#pragma region search

/* the function returns the extension of a given filename*/
char *get_filename_ext(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return "";
	return dot + 1;
}
/* 
the function is called if -r is specifed in the input
it searches the current working directory and also its 
subdirectories for the given str
*/
void searchSubDir(const char *name, int indent, char *str)
{
	DIR *dir;
	struct dirent *entry;
	char *filename;
	FILE *fp;
	int line_num;
	int find_result = 0;
	char temp[512];
	char slash[50] = "";
	// checking if we can open the directory 
	if (!(dir = opendir(name)))
		return;
	// Iterate until there are no more entries to read in the directory 
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_DIR)
		{ // if it is a dirctory
			char path[1024];
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			searchSubDir(path, indent + 2, str);
		}
		else
		{ //if it is a file just search like in the search function
			if (!strcmp(get_filename_ext(entry->d_name), "h") ||
				!strcmp(get_filename_ext(entry->d_name), "c") ||
				!strcmp(get_filename_ext(entry->d_name), "H") ||
				!strcmp(get_filename_ext(entry->d_name), "C"))
			{
				filename = entry->d_name;
				strcpy(slash, name);
				strcat(slash, "/");
				strcat(slash, filename);
				fp = fopen(slash, "r");

				if (fp == NULL)
				{
					fprintf(stderr, "Error while opening the file.\n");
					exit(1);
				}
				line_num = 1;
				// the main algorithm of searching, opens the file and search in it 
				while (fgets(temp, 512, fp) != NULL)
				{
					if ((strstr(temp, str)) != NULL)
					{
						printf("\t%d: %s -> %s", line_num, slash, temp);
						find_result++;
					}
					line_num++;
				}
				//Close the file if still open.
				if (fp)
				{
					fclose(fp);
				}
			} // end of if
		}
	}
	// close directory if still open.
	closedir(dir);
}

/*
the Function searches in all the files in the current working directory 
in all the files with specifed extensions for a specific given word
*/
int search(char *string, int R)
{
	char *filename;
	DIR *d, *sd;
	struct dirent *dir;
	FILE *fp;
	int line_num;
	int find_result = 0;
	char temp[512];
	d = opendir(".");
	char cwd[PATH_MAX];
	// call searchSubDir if -r is specifed 
	if (R == 0)
	{ // run with option -r
		searchSubDir(".", 0, string);
	}
	else
	{ // run without option -r
		if (d)
		{ // if the directory is open
			while ((dir = readdir(d)) != NULL)
			{ // iterate until there is not me files to open
				if (!strcmp(get_filename_ext(dir->d_name), "h") ||
					!strcmp(get_filename_ext(dir->d_name), "c") ||
					!strcmp(get_filename_ext(dir->d_name), "H") ||
					!strcmp(get_filename_ext(dir->d_name), "C"))
				{ // checking the files extension before searching
					filename = dir->d_name;
					getcwd(cwd, sizeof(cwd));
					strcat(cwd, "/");
					strcat(cwd, filename);
					fp = fopen(filename, "r");

					if (fp == NULL)
					{ // if the file couldn't open
						fprintf(stderr, "Error whil opening the file.\n");
						return 1;
					}
					line_num = 1;
					// the main algorithm of searching, opens the file and search in
					// it for the specifed string.
					while (fgets(temp, 512, fp) != NULL)
					{
						if ((strstr(temp, string)) != NULL)
						{
							printf("\t%d: ./%s -> %s", line_num, filename, temp);
							find_result++;
						}
						line_num++;
					}
					//Close the file if still open.
					if (fp)
					{
						fclose(fp);
					}
				} // end of if
			}	  // end of while
			// close the directory if it is still open
			closedir(d);
		} // end of if d
	} // end of if option -r
}

#pragma endregion
/*************************************** Main ***************************************/

/*
the function takes the entered arguments and return their Enviroment Path
it takes the background flag to remove & charcter from args
*/
char *find_given_command(char *args[], int background)
{
	int i = 0;
	char *ch, ampersand[] = "&";
	if (background)
	{ // if background flag is true then remove & character
		while (&ch != NULL)
		{
			ch = args[i];
			if (!strcmp(ch, ampersand))
			{
				break;
			}
			i++;
		}
		args[i] = '\0';
	}
	char *path = getenv("PATH");
	FILE *file;
	char *token = strtok(path, ":");
	char *my_command = args[0];
	char *filename;
	int is_exists = 0;
	while (token != NULL)
	{ // tokenizing the PATH and returning the filename paths
		filename = malloc(strlen(token) + strlen(my_command) + 2);
		strcpy(filename, token);
		strcat(filename, "/");
		strcat(filename, my_command);

		if (file = fopen(filename, "r"))
		{
			is_exists = 1;
			return filename;
		}
		token = strtok(NULL, ":");
	}
}

int main(void)
{
	char inputBuffer[MAX_LINE] = {0}; /*buffer to hold command entered */
	int background;					  /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE / 2 + 1];	  /*command line arguments */
	int count;						  // count of items in args
	int last;						/*the index of the last character in the search phrase, used in calling search*/
	pid_t foregroundProcess = 0;  // holds the foreground process pid
	struct sigaction action;  /*the action to change ^z response*/
	int status_sig;				/*the signal status*/
 	signal(SIGTSTP, tstp_handler);  /* calling signal for the ^z functionality*/

	while (1)
	{
		background = 0;
		count = 0;
		printf("myshell: ");
		/*setup() calls exit() when Control-D is entered */
		setup(inputBuffer, args, &background);

		if (inputBuffer[0] == NULL)  // if input is empty ignore it
			continue;
		while (args[count] != NULL)
		{ // counting items in args
			count++;
		}
		if (!strcmp(args[0], "ps_all"))
		{ // calling ps_all
			ps_all();
		}
		else if (!strcmp(args[0], "search"))
		{
			if (count == 3) // with -r in args[1]
			{ // if -r is set then count is 3
				last = strlen(args[2]) - 1;
				/* removing quotation marks from input string*/
				memmove(&args[2][last], &args[2][last + 1], strlen(args[2]) - last);
				memmove(&args[2][0], &args[2][0 + 1], strlen(args[2]) - 0);
				if (!strcmp(args[1], "-r"))
				{ // if nothing is wrong with args then call search
					search(args[2], 0);
				} else {
					fprintf(stderr, "Usage : search -r \"word to be searched\"");
       	 	}		
			}
			else if (count == 2) 
			{ // if input without -r
				last = strlen(args[1]) - 1;
				/* removing quotation marks from input string*/
				memmove(&args[1][last], &args[1][last + 1], strlen(args[1]) - last);
				memmove(&args[1][0], &args[1][0 + 1], strlen(args[1]) - 0);
				search(args[1], 1);
			} else {
        		fprintf(stderr, "Usage : search \"word to be searched\"");
      		}
		}
		else if (!strcmp(args[0], "bookmark"))
		{
			if (count >= 2)  // this has to be true otherwise invalid input
			{
				int last_ch = strlen(args[count - 1]);
				if (!strcmp(args[1], "-l"))  // list all the saved bookmarks
				{
					printBookmark();
				}
				else if (!strcmp(args[1], "-d"))  // delete the bookmark at the specified index
				{
					deleteBookmark(atoi(args[2]));
				}
				else if (args[1][0] == '"' && args[count - 1][last_ch - 1] == '"')  // save the new bookmark
				{
					args[count - 1][last_ch - 1] = '\0';
					args[1]++;
					addBookmark(args);
				}
				else if (!strcmp(args[1], "-i"))  // execute the new bookmark
				{
					bookmark *cmd = executeBookmark(atoi(args[2]));
					if (cmd != NULL)
						system(cmd->args);
					else
						fprintf(stderr, "No bookmark at this index!\n");
				}
			}
			else
			{
				fprintf(stderr, "Invalid input for bookmark command!");
			}
		} else if (!strcmp(args[0], "exit"))
		{ // calling exit
			process *cursor = process_list;
			int running = 0;
			cursor = process_list;
			while (cursor != NULL)  
			{ // iterate over the processes list and update their status if a new one is available
				if (waitpid(cursor->pid, NULL, WNOHANG) == (cursor->pid))
				{
					cursor->completed = 1;
				}
				else
				{
					cursor->completed = 0;
				}
				cursor = cursor->next;
			}
			cursor=process_list;
			while (cursor != NULL)  // check if any of the processes are still running
			{
				if (!cursor->completed)
				{
					running = 1;
				}
				cursor = cursor->next;
			}
			if (running)  // if running tell the user he cant exit
			{
				printf("Warning: There are still running processes!\n");
			}
			else
			{
				exit(1);
			}
		}
		else
		{
			int i = 0, k = 0;
			char *ch = "";
			char *tempBuffer[MAX_COMMAND_LEN] = {0};
			pid_t pid;
			int in = stdin, out = stdout, err = stderr;  // resetting the I/O channels just to be sure
			char *files[3] = {0}; // INFILE, OUTFILE, ERRFILE
			int *FLAGS[5] = {0};  // PIPE_FLAG, TRUNC_FLAG, APPEND_FLAG, IN_FLAG, ERR_FLAG
			while (args[i] != NULL)  // iterate over the args and build the commands
			{
				ch = args[i];
				if (!strcmp(ch, "|"))
				{
					FLAGS[0] = 1; // set pipe flag
					tempBuffer[k] = args[i];
					//i++;
					k++;
				}
				else if (!strcmp(ch, ">"))
				{
					FLAGS[1] = 1; // set trunc flag
					i++;
					files[1] = args[i];  // set output file
				}
				else if (!strcmp(ch, ">>"))
				{
					FLAGS[2] = 1; // set append flag
					i++;
					files[1] = args[i];  // set output file
				}
				else if (!strcmp(ch, "<"))
				{
					FLAGS[3] = 1; // set in flag
					i++;
					files[0] = args[i];  // set input file
				}
				else if (!strcmp(ch, "2>"))
				{
					FLAGS[4] = 1; // set err flag
					i++;
					files[3] = args[i]; // set error file
				}
				else
				{
					tempBuffer[k] = args[i];  // if not a special char just add it to temp buffer
					k++;
				}
				i++;
			}

			pid = fork();  // create a new child process

			if (pid == 0)  // this is the child
			{
				if (!background) {  // if its a foreground process set the signal handling actions
					foregroundProcess = getpid();
					action.sa_handler = tstp_handler;
					action.sa_flags = 0;
					status_sig = sigemptyset(&action.sa_mask);
				
					if (status_sig == -1) {
						fprintf(stderr, "Error");
						exit(1);
					}
					status_sig = sigaction(SIGTSTP, &action, NULL);
					if (status_sig == -1) {
						fprintf(stderr,"errors");
						exit(1);
					}
				} 
				pid_t childpid = getpid();  // fo adding to the processes linked list
				
				if ((FLAGS[0] == 0) && (FLAGS[1] == 0) && (FLAGS[2] == 0) && (FLAGS[3] == 0) && (FLAGS[4] == 0))
				{  // if none of our special case flags (pipeline, I/O redirection) are set run it with execv
					char *path;
					path = find_given_command(args, background);  // search for the path of the command
					// execv(path, args);
					if (execv(path, args) == -1){  // if the execv call is not succesful close the created process
						perror("exev");  // print the error to stderr
						exit(1);  // exit the new process so we dont have the shell running multiple times
					}
				}
				else  // if we have a pipeline or I/O redirection
				{
					int i = 0;
                    char *res = malloc(sizeof(char) * 100);
                    while (tempBuffer[i] != NULL)
                    {
                        //printf("in while %s", tempBuffer[i]);
                        char *tmp = malloc(sizeof(char) * sizeof(tempBuffer[i]));
                        memcpy(tmp, tempBuffer[i], sizeof(tempBuffer[i]) + 1);
                        strcat(res, tmp);
                        strcat(res, " ");
                        i++;
                    }
					// char *res = concatenate_args(tempBuffer);  // concatenate the tempBuffer so we can give it to system() function 
					if (FLAGS[4] == 1)  // if there is error redirection
					{ // error
						err = open(files[2], CREATE_TRUNC_FLAGS, CREATE_MODE);
						if (err == -1)
						{
							perror("Failed to open error file");
							return;
						}
						if (dup2(err, STDERR_FILENO) == -1)
						{
							perror("Failed to open error file");
							return;
						}
						if (close(err) == -1)
						{
							perror("Failed to open error file");
							return;
						}
					}

					// set infile
					if (FLAGS[3] == 1)  // if there is input redirection
					{
						in = open(files[0], READ_FLAGS);
						if (in == -1)
						{
							perror("Failed to open infile.txt");
							return;
						}
						if (dup2(in, STDIN_FILENO) == -1)
						{
							perror("Failed to read standard input");
							return;
						}
						if (close(in) == -1)
						{
							perror("Failed to close the infile");
							return;
						}
					}

					// set outfile
					if ((FLAGS[1] == 1) || (FLAGS[2] == 1))   // if there is output redirection
					{
						if ((FLAGS[1] == 1) && (FLAGS[2] != 1))
						{ // truncate
							out = open(files[1], CREATE_TRUNC_FLAGS, CREATE_MODE);
							if (out == -1)
							{
								perror("Failed to open outfile.txt");
								return;
							}
							if (dup2(out, STDOUT_FILENO) == -1)
							{
								perror("Failed to redirect standard output");
								return;
							}
							if (close(out) == -1)
							{
								perror("Failed to close the outfile");
								return;
							}
						}
						else if ((FLAGS[1] != 1) && (FLAGS[2] == 1))
						{ // append
							out = open(files[1], CREATE_APPEND_FLAGS, CREATE_MODE);
							if (out == -1)
							{
								perror("Failed to open outfile.txt");
								return;
							}
							if (dup2(out, STDOUT_FILENO) == -1)
							{
								perror("Failed to redirect standard output");
								return;
							}
							if (close(out) == -1)
							{
								perror("Failed to close the outfile");
								return;
							}
						}
						else
						{
							perror("Invalid output redirection!");
							return;
						}
					}
					system(res);  // execute the command with system call
					exit(1);  // exit the new process so we dont have multiple shells running
				}
			}
			else if (pid < 0)
			{
				/* The fork failed.  */
				perror("fork");
				exit(1);
			}
			else
			{
				if (!background)
				{
					wait(NULL);  // if the new process is a foreground process wait for it
				}else{
					char *process_args = concatenate_args(args);
					add_my_process(pid, background, process_args);  // add the background process to our linked list
				}
			}
		}
	} // end of while
}