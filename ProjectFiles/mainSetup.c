#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <libgen.h>
#include <dirent.h>
#include <signal.h>

#define PATH_MAX 4096
#define ERROR_CHECK_ARGUMENTS "Check your arguments!\n"
#define ERROR_OPEN_DIRECTORY "Did not open current directory\n"
#define ERROR_GETCWD "Error getcwd()\n"
#define ERROR_TWO_WAYS "You should use these commands:\nsearch 'command'\nsearch 'option' 'command'\n"

struct listProcess
{

	long processNumber;
	pid_t pid;
	char progName[50];
	struct listProcess *nextPtr;
};
typedef struct listProcess ListProcess;
typedef ListProcess *ListProcessPtr;

struct bookmark
{

	char progName[50];
	struct bookmark *nextPtr;
};
typedef struct bookmark bookmarks;
typedef bookmarks *bookmarkPtr;

void setup(char inputBuffer[], char *args[], int *background);
long findpathof(char *pth, const char *exe);
void insert(ListProcessPtr *sPtr, pid_t pid, char progName[]);
void insertBookmark(bookmarkPtr *bPtr, char progName[]);
void deleteStoppedList(ListProcessPtr *currentPtr);
void killAllChildProcess(pid_t ppid);
void childSignalHandler(int signum);
void sigtstpHandler();
void createProcess(char path[], char *args[], int *background, ListProcessPtr *sPtr);
void bookmarkCommand(char *args[], bookmarkPtr *startPtrBookmark);
void printSearchCommand(char *fileName, char *pattern);
void listFilesRecursively(char *basePath, char *pattern);
void formatInput(char *args[]);
void searchCommand(char *args[]);
long checkIORedirection(char *args[]);
int main(void);

char inputFileName[20];
char outputFileName[20];
char outputRedirectSymbol[3] = {"00"};

long inputRedirectFlag;
long outputRedirectFlag;

long numOfArgs = 0;
long processNumber = 1;

pid_t parentPid;
pid_t fgProcessPid = 0;

void setup(char inputBuffer[], char *args[], int *background)
{
	int length,
		i,
		start,
		ct;

	ct = 0;

	length = read(STDIN_FILENO, inputBuffer, 90);

	start = -1;
	if (length == 0)
		exit(0);

	if ((length < 0) && (errno != EINTR))
	{
		fprintf(stderr, "%s", "Reading the error\n");
		exit(-1);
	}

	for (i = 0; i < length; i++)
	{

		switch (inputBuffer[i])
		{
		case ' ':
		case '\t':
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			start = -1;
			break;

		case '\n':
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL;
			break;

		default:
			if (start == -1)
				start = i;
			if (inputBuffer[i] == '&')
			{
				*background = 1;
				inputBuffer[i - 1] = '\0';
			}
		}
	}
	args[ct] = NULL;
	numOfArgs = ct;
}

long findpathof(char *pth, const char *exe)
{
	char *searchpath;
	char *beg;
	char *end;
	long stop;
	long found;
	long len;

	if (strchr(exe, '/') == NULL)
	{
		searchpath = getenv("PATH");
		if (searchpath == NULL)
			return 0;
		if (strlen(searchpath) <= 0)
			return 0;

		beg = searchpath;
		stop = 0;
		found = 0;
		do
		{
			end = strchr(beg, ':');

			if (end != NULL)
			{
				strncpy(pth, beg, end - beg);
				pth[end - beg] = '\0';
				len = end - beg;
			}
			else
			{
				stop = 1;
				strncpy(pth, beg, PATH_MAX);
				len = strlen(pth);
			}
			if (pth[len - 1] != '/')
				strncat(pth, "/", 2);
			strncat(pth, exe, PATH_MAX - len);

			long result;
			struct stat statinfo;

			result = stat(pth, &statinfo);
			if (result < 0)
				found = 0;
			if (!S_ISREG(statinfo.st_mode))
				found = 0;
			if (statinfo.st_uid == geteuid())
				found = statinfo.st_mode & S_IXUSR;
			if (statinfo.st_gid == getegid())
				found = statinfo.st_mode & S_IXGRP;
			found = statinfo.st_mode & S_IXOTH;

			if (!stop)
				beg = end + 1;
		} while (!stop && !found);

		return found;
	}

	if (realpath(exe, pth) != NULL)
	{
		long result;
		struct stat statinfo;

		result = stat(pth, &statinfo);
		if (result < 0)
			return 0;
		else if (!S_ISREG(statinfo.st_mode))
			return 0;
		else if (statinfo.st_uid == geteuid())
			return statinfo.st_mode & S_IXUSR;
		else if (statinfo.st_gid == getegid())
			return statinfo.st_mode & S_IXGRP;
		return statinfo.st_mode & S_IXOTH;
	}
	else
	{
		return 0;
	}
}

void insert(ListProcessPtr *sPtr, pid_t pid, char progName[])
{

	ListProcessPtr newPtr = malloc(sizeof(ListProcess));

	if (newPtr == NULL)
	{
		fprintf(stderr, "%s", "Memory did not use now\n");
	}
	else
	{
		strcpy(newPtr->progName, progName);
		newPtr->processNumber = processNumber;
		newPtr->pid = pid;
		newPtr->nextPtr = NULL;

		ListProcessPtr previousPtr = NULL;
		ListProcessPtr currentPtr = *sPtr;

		while (currentPtr != NULL)
		{
			previousPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}

		if (previousPtr != NULL)
		{
			previousPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
		}
		else
		{
			newPtr->nextPtr = *sPtr;
			*sPtr = newPtr;
		}
	}
}

void insertBookmark(bookmarkPtr *bPtr, char progName[])
{

	bookmarkPtr newPtr = malloc(sizeof(bookmarks));

	if (newPtr == NULL)
	{
		fprintf(stderr, "%s", "Memory did not use now\n");
	}
	else
	{
		strcpy(newPtr->progName, progName);
		newPtr->nextPtr = NULL;

		bookmarkPtr previousPtr = NULL;
		bookmarkPtr currentPtr = *bPtr;

		while (currentPtr != NULL)
		{
			previousPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}

		if (previousPtr != NULL)
		{
			previousPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
		}
		else
		{
			newPtr->nextPtr = *bPtr;
			*bPtr = newPtr;
		}
	}
}

void deleteStoppedList(ListProcessPtr *currentPtr)
{
	int status;

	if ((*currentPtr) != NULL)
	{

		if (waitpid((*currentPtr)->pid, &status, WNOHANG) != -1)
		{
			ListProcessPtr previousPtr = *currentPtr;
			ListProcessPtr tempPtr = (*currentPtr)->nextPtr;

			while (tempPtr != NULL && waitpid(tempPtr->pid, &status, WNOHANG) != -1)
			{
				previousPtr = tempPtr;
				tempPtr = tempPtr->nextPtr;
			}
			if (tempPtr == NULL)
			{
			}
			else
			{
				ListProcessPtr delPtr = tempPtr;
				previousPtr->nextPtr = tempPtr->nextPtr;
				free(delPtr);
				deleteStoppedList(currentPtr);
			}
		}
		else
		{
			ListProcessPtr tempPtr = *currentPtr;
			*currentPtr = (*currentPtr)->nextPtr;
			free(tempPtr);
			deleteStoppedList(currentPtr);
		}
	}
	else
	{
		return;
	}
}

void killAllChildProcess(pid_t ppid)
{
	char *buff = NULL;
	size_t len = 255;
	char command[256] = {0};

	sprintf(command, "ps -ef|awk '$3==%u {print $2}'", ppid);
	FILE *fp = (FILE *)popen(command, "r");
	while (getline(&buff, &len, fp) >= 0)
	{
		killAllChildProcess(atoi(buff));
		char cmd[256] = {0};
		sprintf(cmd, "kill -TSTP %d", atoi(buff));
		system(cmd);
	}
	free(buff);
	fclose(fp);
}

void childSignalHandler(int signum)
{
	int status;
	pid_t pid;
	pid = waitpid(-1, &status, WNOHANG);
}

void sigtstpHandler()
{

	if (fgProcessPid == 0 || waitpid(fgProcessPid, NULL, WNOHANG) == -1)
	{
		printf("\nmyshell: ");
		fflush(stdout);
		return;
	}

	killAllChildProcess(fgProcessPid);
	char cmd[256] = {0};
	sprintf(cmd, "kill -TSTP %d", fgProcessPid);
	system(cmd);
	fgProcessPid = 0;
}

void createProcess(char path[], char *args[], int *background, ListProcessPtr *sPtr)
{

	pid_t childPid;

	childPid = fork();

	if (childPid == 0)
	{
		if (inputRedirectFlag == 1)
		{

			long fdInput;
			fdInput = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

			if (fdInput != -1)
			{
			}
			else
			{
				fprintf(stderr, "%s", "File is did not open\n");
				return;
			}

			if (dup2(fdInput, STDIN_FILENO) != -1)
			{
			}
			else
			{
				fprintf(stderr, "%s", "Failed to redirect standard input...\n");
				return;
			}

			if (close(fdInput) != -1)
			{
			}
			else
			{
				fprintf(stderr, "%s", "�nput file not closed\n");
				return;
			}

			if (outputRedirectFlag == 1)
			{
				long fdOutput;

				if (strcmp(outputRedirectSymbol, "2>") == 0)
				{
					fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

					if (fdOutput != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(fdOutput, STDERR_FILENO) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to redirect standard error...\n");
						return;
					}
				}
				else if (strcmp(outputRedirectSymbol, ">>") == 0)
				{
					fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

					if (fdOutput != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(fdOutput, STDOUT_FILENO) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to redirect standard output...\n");
						return;
					}
				}
				else if (strcmp(outputRedirectSymbol, ">") == 0)
				{
					fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

					if (fdOutput != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(fdOutput, STDOUT_FILENO) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to redirect standard output...\n");
						return;
					}
				}
			}
		}
		else if (outputRedirectFlag == 1)
		{
			long fdOutput;

			if (strcmp(outputRedirectSymbol, "2>") == 0)
			{
				fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

				if (fdOutput != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
					return;
				}

				if (dup2(fdOutput, STDERR_FILENO) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to redirect standard error...\n");
					return;
				}
			}
			else if (strcmp(outputRedirectSymbol, ">>") == 0)
			{
				fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

				if (fdOutput != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
					return;
				}

				if (dup2(fdOutput, STDOUT_FILENO) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to redirect standard output...\n");
					return;
				}
			}
			else if (strcmp(outputRedirectSymbol, ">") == 0)
			{
				fdOutput = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

				if (fdOutput != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
					return;
				}

				if (dup2(fdOutput, STDOUT_FILENO) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to redirect standard output...\n");
					return;
				}
			}
		}
		execv(path, args);
	}
	else if (childPid == -1)
	{
		fprintf(stderr, "%s", "Failed fork() fucntion!\n");
		return;
	}
	else
	{
		if (*background != 1)
		{
			setpgid(childPid, childPid);
			fgProcessPid = childPid;
			if (childPid != waitpid(childPid, NULL, WUNTRACED))
				fprintf(stderr, "%s", "Parent failed due to a signal or error while waiting for her child !!!\n");
		}
		else
		{
			waitpid(childPid, NULL, WNOHANG);
			setpgid(childPid, childPid);
			insert(&(*sPtr), childPid, args[0]);
			processNumber++;
		}
	}
}

void bookmarkCommand(char *args[], bookmarkPtr *startPtrBookmark)
{

	char *tempStringComp = "\"";
	long arg2IsInt = 0;

	long i = 0;
	while (args[i] != NULL)
	{
		i++;
	}

	if (((strcmp(args[1], "-i") == 0) || (strcmp(args[1], "-d") == 0)) && i == 3)
	{
		char *temp = args[2];

		long length, index;
		length = strlen(temp);
		for (index = 0; index < length; index++)
			if (!isdigit(temp[index]))
			{
				fprintf(stderr, "%s", "Check arguments !\n");
				arg2IsInt = 1;
			}
		if (arg2IsInt == 1)
		{
			arg2IsInt = 0;
		}
	}

	if ((strcmp(args[1], "-d") == 0) && i == 3)
	{

		if (arg2IsInt != 0)
		{
			return;
		}
		else
		{
			bookmarkPtr *tempPointer = startPtrBookmark;
			long index = atoi(args[2]);

			if (*tempPointer != NULL)
			{

				if (index != 0)
				{
					bookmarkPtr previousPtr = *tempPointer;
					bookmarkPtr tempPtr = (*tempPointer)->nextPtr;
					long temp = 1;

					while (temp != index && tempPtr != NULL)
					{
						previousPtr = tempPtr;
						tempPtr = tempPtr->nextPtr;
						temp++;
					}
					if (tempPtr != NULL)
					{
						bookmarkPtr delPtr = tempPtr;
						previousPtr->nextPtr = tempPtr->nextPtr;
						free(delPtr);
					}
					else
					{
						fprintf(stderr, "%s", "This directory is wrong directory.\n");
					}
				}
				else
				{
					bookmarkPtr tempPtr = *tempPointer;
					*tempPointer = (*tempPointer)->nextPtr;
					free(tempPtr);
				}
			}
			else
			{
				fprintf(stderr, "%s", "Empty list\n");
			}
			return;
		}
	}
	else if ((strcmp(args[1], "-i") == 0) && i == 3)
	{

		if (arg2IsInt != 0)
		{
			return;
		}
		else
		{
			long index = atoi(args[2]);
			char *progpath;

			if (*startPtrBookmark != NULL)
			{
				bookmarkPtr tempPtr = *startPtrBookmark;
				long j = 0;
				while (tempPtr != NULL && j != index)
				{
					tempPtr = tempPtr->nextPtr;
					j++;
				}
				if (tempPtr != NULL)
				{
					char exe[90];
					strcpy(exe, tempPtr->progName);
					long length = strlen(exe);
					long i = 0;
					exe[length - 2] = '\0';
					for (i = 0; i < length; i++)
					{
						exe[i] = exe[i + 1];
					}
					char command[100];
					sprintf(command, "%s", exe);
					system(command);
				}
				else
				{
					fprintf(stderr, "%s", "This directory is wrong directory.\n");
				}
			}
			else
			{
				fprintf(stderr, "%s", "Empty list");
			}
			return;
		}
	}
	else if ((strcmp(args[1], "-l") == 0) && i == 2)
	{

		long count = 0;
		bookmarkPtr tempPointer = *startPtrBookmark;

		if (*startPtrBookmark != NULL)
		{
			while (tempPointer->nextPtr != NULL)
			{
				printf("%ld %s\n", count, tempPointer->progName);
				count++;
				tempPointer = tempPointer->nextPtr;
			}
			printf("%ld %s\n", count, tempPointer->progName);
		}
		else
		{
			fprintf(stderr, "%s", "List is empty\n");
		}
	}
	else if ((strcmp(args[1], "-h") == 0) && i == 2)
	{
		printf("try again");
		return;
	}
	else if (i == 1)
	{
		fprintf(stderr, "Bookmark usage wrong! You can type \"bookmark -h\" to see the correct usage.\n");
		return;
	}
	else if (strlen(args[1]) < strlen(tempStringComp) ? 0 : memcmp(tempStringComp, args[1], strlen(tempStringComp)) == 0)
	{

		long length = strlen(args[numOfArgs - 1]);
		char command[100];
		strcpy(command, args[numOfArgs - 1]);

		if (command[length - 1] == '\"')
		{
		}
		else
		{
			fprintf(stderr, "%s", "Bookmark usage wrong ");
			return;
		}

		char *exec;
		char path[PATH_MAX + 1];
		char firstArgument[50];
		long lengthOfFirstArgument = strlen(args[numOfArgs - 1]);

		strcpy(firstArgument, args[1]);
		long t = 0;

		if (firstArgument[0] == '\"' && firstArgument[lengthOfFirstArgument - 1] != '\"')
		{
			for (t = 0; t < lengthOfFirstArgument - 1; t++)
			{
				firstArgument[t] = firstArgument[t + 1];
			}
			firstArgument[lengthOfFirstArgument - 1] = '\0';
		}
		else if (firstArgument[0] == '\"' && firstArgument[lengthOfFirstArgument - 1] == '\"')
		{
			firstArgument[lengthOfFirstArgument - 1] = '\0';

			for (t = 0; t < lengthOfFirstArgument - 1; t++)
			{
				firstArgument[t] = firstArgument[t + 1];
			}
		}
		exec = firstArgument;

		if (findpathof(path, exec))
		{
		}
		else
		{
			fprintf(stderr, "%s", "There is not such a command  !\n");
			return;
		}

		char exe[90] = {""};
		for (t = 1; t < numOfArgs; t++)
		{
			strcat(exe, args[t]);
			strcat(exe, " ");
		}
		pid_t tempPid;
		insertBookmark(startPtrBookmark, exe);
		exe[0] = '\0';
	}
	else
	{
		fprintf(stderr, "%s", "Bookmark usage wrong! ");
		return;
	}
}

void printSearchCommand(char *fileName, char *pattern)
{

	char file[1000] = {0};

	char *buff = NULL;
	char fName[256] = {0};
	size_t len = 255;

	sprintf(fName, "grep -rnwl  %s -e %s | awk '{print $0}'", fileName, pattern);
	FILE *fp = (FILE *)popen(fName, "r");

	while (getline(&buff, &len, fp) >= 0)
	{
		strcpy(file, buff);
	}
	free(buff);
	fclose(fp);

	char tempFileName[1000];
	strcpy(tempFileName, file);

	char allLine[256] = {0};

	char result[1024];

	char *buff2 = NULL;
	char command[256] = {0};
	size_t len2 = 256;

	sprintf(command, "grep -rnw  %s -e %s | awk '{print $0}'", fileName, pattern);

	FILE *fp2 = (FILE *)popen(command, "r");
	while (fgets(result, sizeof(result), fp2))
	{
		strcpy(allLine, result);

		char lineNumber[15] = {0};

		long i = 0;

		long length = strlen(allLine);
		long digitNum = 1;

		for (i = 0; i < length; i++)
		{
			if (!isdigit(allLine[i]))
			{
				break;
			}
			else
			{
				lineNumber[i] = allLine[i];
				digitNum++;
			}
		}

		for (i = 0; i < length; i++)
		{
			allLine[i] = allLine[digitNum + i];
		}

		if (strlen(file) >= 1 && isdigit(lineNumber[0]))
		{
			file[strlen(file) - 1] = '\0';

			printf("%s : %s -> %s", lineNumber, file, allLine);
			strcpy(file, tempFileName);
		}
	}

	free(buff2);
	fclose(fp2);
}

void listFilesRecursively(char *basePath, char *pattern)
{
	char path[1000];
	struct dirent *dp;
	DIR *dir = opendir(basePath);

	if (dir)
	{
		while ((dp = readdir(dir)) != NULL)
		{
			if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
			{

				char fName[50];

				strcpy(fName, dp->d_name);
				char grepFile[1000];
				strcpy(grepFile, basePath);
				strcat(grepFile, "/");
				strcat(grepFile, dp->d_name);

				if (fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
														fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H'))
				{
					printSearchCommand(grepFile, pattern);
				}
				strcpy(path, basePath);
				strcat(path, "/");
				strcat(path, dp->d_name);
				listFilesRecursively(path, pattern);
			}
		}
		closedir(dir);
	}
	else
	{
		return;
	}
}

void searchCommand(char *args[])
{
	long i = 0;
	while (args[i] != NULL)
	{
		i++;
	}

	if (i < 2 || i > 3)
	{
		fprintf(stderr, "You should use these commands:\nsearch 'command'\nsearch 'option' 'command'\n");
		return;
	}

	if (i == 3 && strcmp(args[1], "-r") != 0)
	{
		fprintf(stderr, "Check your arguments!!\n");
		return;
	}

	if (i == 2 || (i == 3 && strcmp(args[1], "-r") == 0))
	{
		char cmd[1000];
		struct dirent *de;
		DIR *dr = opendir(".");

		if (dr == NULL)
		{
			fprintf(stderr, "Did not open current directory\n");
			return;
		}

		while ((de = readdir(dr)) != NULL)
		{
			if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
			{
				char fName[50];
				strcpy(fName, de->d_name);

				if (fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
														fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H'))
				{
					long length = strlen(args[i - 1]);
					char pattern[100];
					strcpy(pattern, args[i - 1]);

					if (!(pattern[0] == '"' && pattern[length - 1] == '"'))
					{
						fprintf(stderr, "Check your arguments!You need to give your pattern between \" \"\n");
						closedir(dr);
						return;
					}

					printSearchCommand(de->d_name, args[i - 1]);
				}
			}
		}

		printf("\n");
		closedir(dr);
	}
	else
	{
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL)
		{
			listFilesRecursively(cwd, args[2]);
		}
		else
		{
			fprintf(stderr, "Error getcwd()\n");
		}
	}
}

void formatInput(char *args[])
{

	long i = 0;
	long a;
	long counter;
	long flag = 0;
	for (i = 0; i < numOfArgs; i++)
	{
		if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0)
		{
			args[i] = NULL;
			a = i;
			counter = i + 1;
			flag = 1;
			break;
		}
	}

	if (flag != 0)
	{
		for (i = counter; i < numOfArgs; i++)
		{
			args[i] = NULL;
		}

		numOfArgs = numOfArgs - (numOfArgs - a);
	}
	else
	{
		return;
	}
}

long checkIORedirection(char *args[])
{
	if (numOfArgs == 2 && strcmp(args[0], "io") == 0 && strcmp(args[1], "-h") == 0)
	{
		printf("[1] -> \"myprog [args] > file.out\"\n");
		printf("[2] -> \"myprog [args] >> file.out\"\n");
		printf("[3] -> \"myprog [args] < file.in\"\n");
		printf("[4] -> \"myprog [args] 2> file.out\"\n");
		return 1;
	}

	long a;
	long io;
	for (a = 0; a < numOfArgs; a++)
	{
		if (strcmp(args[a], "<") == 0 || strcmp(args[a], ">") == 0 || strcmp(args[a], ">>") == 0 || strcmp(args[a], "2>") == 0)
		{
			io = 1;
		}
	}
	if (io == 1)
	{

		if (strcmp(args[0], "bookmark") == 0 || strcmp(args[0], "search") == 0)
		{
			fprintf(stderr, "I/O redirection is not valid for \" %s \" command!!\n", args[0]);
			return 1;
		}
	}
	else
	{
		return 0;
	}

	long i;
	for (i = 0; i < numOfArgs; i++)
	{
		if (strcmp(args[i], "<") == 0 && numOfArgs > 3 && strcmp(args[i + 2], ">") == 0)
		{
			if (i + 3 >= numOfArgs)
			{
				fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
				args[0] = NULL;
				return 1;
			}

			inputRedirectFlag = 1;
			outputRedirectFlag = 1;
			strcpy(outputRedirectSymbol, args[i + 2]);
			strcpy(inputFileName, args[i + 1]);
			strcpy(outputFileName, args[i + 3]);

			return 0;
		}
		else if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0)
		{
			if (i + 1 >= numOfArgs)
			{
				fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
				args[0] = NULL;
				return 1;
			}

			outputRedirectFlag = 1;
			strcpy(outputRedirectSymbol, args[i]);
			strcpy(outputFileName, args[i + 1]);

			return 0;
		}
		else if (strcmp(args[i], "<") == 0)
		{
			if (i + 1 >= numOfArgs)
			{
				fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
				args[0] = NULL;
				return 1;
			}

			inputRedirectFlag = 1;
			strcpy(inputFileName, args[i + 1]);

			return 0;
		}
	}
}

int main(void)
{

	char inputBuffer[90];
	int background;
	char *args[80];
	char path[PATH_MAX + 1];
	char *progpath;
	char *exe;

	system("clear");

	signal(SIGCHLD, childSignalHandler);
	signal(SIGTSTP, sigtstpHandler);

	parentPid = getpid();

	ListProcessPtr startPtr = NULL;
	bookmarkPtr startPtrBookmark = NULL;

	while (parentPid == getpid())
	{
		background = 0;
		if (startPtr == NULL)
			processNumber = 1;
		printf("myshell: ");
		fflush(0);

		setup(inputBuffer, args, &background);

		if (args[0] == NULL)
			continue;
		progpath = strdup(args[0]);
		exe = args[0];

		if (checkIORedirection(args) != 0)
		{
			continue;
		}
		formatInput(args);

		if (strcmp(args[0], "exit") == 0)
		{
			deleteStoppedList(&startPtr);
			if ((startPtr == NULL) != 0)
			{
				exit(1);
			}
			else
			{
				fprintf(stderr, "%s", "Background procces running!\n");
			}
		}
		else if (strcmp(args[0], "search") == 0)
		{
			searchCommand(args);
			continue;
		}
		else if (strcmp(args[0], "bookmark") == 0)
		{
			bookmarkCommand(args, &startPtrBookmark);
			continue;
		}
		else if (!findpathof(path, exe))
		{
			fprintf(stderr, "No executable \"%s\" found\n", exe);
			free(progpath);
		}
		else
		{
			if (*args[numOfArgs - 1] == '&')
				args[numOfArgs - 1] = '\0';
			createProcess(path, args, &background, &startPtr);
		}
		path[0] = '\0';
		inputFileName[0] = '\0';
		outputFileName[0] = '\0';
		inputRedirectFlag = 0;
		outputRedirectFlag = 0;
	}
}
