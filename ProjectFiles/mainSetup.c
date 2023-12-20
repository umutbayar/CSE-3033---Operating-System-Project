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
#define ERROR_CHECK_ARGUMENTS "Check your arguments!!\n"
#define ERROR_OPEN_DIRECTORY "Did not open directory\n"
#define ERROR_GETCWD "Error getcwd() \n"
#define ERROR_TWO_WAYS "2 ways to use this command :\nsearch 'command'\nsearch 'option' 'command'\n"
#define ERROR_MEMORY "Did not use memory\n"
#define EMPTY_LIST "Empty list\n"
#define ERROR_BOOKMARK "Bookmark usage wrong! "

struct listProcess
{
	short testByte;
	long noOfProc;
	pid_t pid;
	char nameOfprog[120];
	struct listProcess *pointNext;
};
typedef struct listProcess ListProcess;
typedef ListProcess *ListProcessPtr;

struct bookmark
{
	short testByte;
	char nameOfprog[50];
	struct bookmark *pointNext;
};
typedef struct bookmark bookmarks;
typedef bookmarks *pointBookmark;

void setup(char inputBuffer[], char *args[], int *background);
long pathFounder(const char *executable, char *Path, int testCondition);
void append(ListProcessPtr *sPtr, pid_t pid, char nameOfprog[], int testCondition);
void appendBM(pointBookmark *PointerB, char nameOfprog[], int testCondition);
void ListKillofStopped(ListProcessPtr *pointOfNow, int testCondition);
void childPkiller(pid_t ppid);
void SignalofCh(int signum);
void pointOfSigts();
void ProcessOfconstract(char path[], char *args[], int *background, ListProcessPtr *sPtr, int testCondition);
void reqOfBmark(char *args[], pointBookmark *startPtrBookmark, int testCondition);
void reqOffindPrinter(char *fileName, char *Design, int testCondition);
void printerOfFolder(char *pathOfStart, char *Design, int testCondition);
void inputOfConfiguration(char *args[], int testCondition);
void reqOffind(char *args[], int testCondition);
long SwitchIOControl(char *args[], int testCondition);
int main(void);

char inputFileName[20];
char outputFileName[20];
char outputRedirectSymbol[3] = {"00"};

long inputRedirectFlag;
long outputRedirectFlag;

long numOfArgs = 0;
long noOfProc = 1;

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
		fprintf(stderr, "%s", "Error command\n");
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

long pathFounder(const char *executable, char *Path, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		char *beg;
		char *end;
		long stop;
		long found;
		long len;

		if (strchr(executable, '/') == NULL)
		{
			if (getenv("PATH") == NULL)
				return 0;
			if (strlen(getenv("PATH")) <= 0)
				return 0;

			beg = getenv("PATH");
			stop = 0;
			found = 0;
			do
			{
				end = strchr(beg, ':');

				if (end != NULL)
				{
					strncpy(Path, beg, end - beg);
					Path[end - beg] = '\0';
					len = end - beg;
				}
				else
				{
					stop = 1;
					strncpy(Path, beg, PATH_MAX);
					len = strlen(Path);
				}
				if (Path[len - 1] != '/')
					strncat(Path, "/", 2);
				strncat(Path, executable, PATH_MAX - len);

				long result;
				struct stat statinfo;

				result = stat(Path, &statinfo);
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

		if (realpath(executable, Path) != NULL)
		{
			struct stat statinfo;

			if (stat(Path, &statinfo) < 0)
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
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void append(ListProcessPtr *sPtr, pid_t pid, char nameOfprog[], int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		ListProcessPtr newPtr = malloc(sizeof(ListProcess));

		if (newPtr == NULL)
		{
			fprintf(stderr, "%s", ERROR_MEMORY);
		}
		else
		{
			strcpy(newPtr->nameOfprog, nameOfprog);
			newPtr->noOfProc = noOfProc;
			newPtr->pid = pid;
			newPtr->pointNext = NULL;

			ListProcessPtr previousPtr = NULL;
			ListProcessPtr pointOfNow = *sPtr;

			for (; pointOfNow != NULL;)
			{
				previousPtr = pointOfNow;
				pointOfNow = pointOfNow->pointNext;
			}

			if (previousPtr != NULL)
			{
				previousPtr->pointNext = newPtr;
				newPtr->pointNext = pointOfNow;
			}
			else
			{
				newPtr->pointNext = *sPtr;
				*sPtr = newPtr;
			}
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void appendBM(pointBookmark *PointerB, char nameOfprog[], int testCondition)
{
    int i;
    for (i=0;i<10 ; i++){
	}
	int test = 0;
	if (test == testCondition)
	{
		pointBookmark newPtr = malloc(sizeof(bookmarks));

		if (newPtr == NULL)
		{
			fprintf(stderr, "%s", ERROR_MEMORY);
		}
		else
		{
			strcpy(newPtr->nameOfprog, nameOfprog);
			newPtr->pointNext = NULL;

			pointBookmark previousPtr = NULL;
			pointBookmark pointOfNow = *PointerB;

			for (; pointOfNow != NULL;)
			{
				previousPtr = pointOfNow;
				pointOfNow = pointOfNow->pointNext;
			}

			if (previousPtr != NULL)
			{
				previousPtr->pointNext = newPtr;
				newPtr->pointNext = pointOfNow;
			}
			else
			{
				newPtr->pointNext = *PointerB;
				*PointerB = newPtr;
			}
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void ListKillofStopped(ListProcessPtr *pointOfNow, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		int status;

		if ((*pointOfNow) != NULL)
		{

			if (waitpid((*pointOfNow)->pid, &status, WNOHANG) != -1)
			{
				ListProcessPtr previousPtr = *pointOfNow;
				ListProcessPtr tempPtr = (*pointOfNow)->pointNext;

				for (; tempPtr != NULL && waitpid(tempPtr->pid, &status, WNOHANG) != -1;)
				{
					previousPtr = tempPtr;
					tempPtr = tempPtr->pointNext;
				}
				if (tempPtr == NULL)
				{
				}
				else
				{
					ListProcessPtr delPtr = tempPtr;
					previousPtr->pointNext = tempPtr->pointNext;
					free(delPtr);
					ListKillofStopped(pointOfNow, 0);
				}
			}
			else
			{
				ListProcessPtr tempPtr = *pointOfNow;
				*pointOfNow = (*pointOfNow)->pointNext;
				free(tempPtr);
				ListKillofStopped(pointOfNow, 0);
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void childPkiller(pid_t ppid)
{
	char *buff = NULL;
	size_t len = 255;
	char command[256] = {0};

	sprintf(command, "ps -ef|awk '$3==%u {print $2}'", ppid);
	FILE *fp = (FILE *)popen(command, "r");
	for (; getline(&buff, &len, fp) >= 0;)
	{
		childPkiller(atoi(buff));
		char cmd[256] = {0};
		sprintf(cmd, "kill -TSTP %d", atoi(buff));
		system(cmd);
	}
	free(buff);
	fclose(fp);
}

void SignalofCh(int signum)
{
	int status;
	pid_t pid;
	pid = waitpid(-1, &status, WNOHANG);
}

void pointOfSigts()
{

	if (fgProcessPid == 0 || waitpid(fgProcessPid, NULL, WNOHANG) == -1)
	{
		printf("\nmyshell: ");
		fflush(stdout);
		return;
	}

	childPkiller(fgProcessPid);
	char cmd[256] = {0};
	sprintf(cmd, "kill -TSTP %d", fgProcessPid);
	system(cmd);
	fgProcessPid = 0;
}

void ProcessOfconstract(char path[], char *args[], int *background, ListProcessPtr *sPtr, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		pid_t childPid = fork();

		if (childPid == 0)
		{
			if (inputRedirectFlag == 1)
			{

				if (open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to open the file given as input...\n");
					return;
				}

				if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDIN_FILENO) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to redirect standard input...\n");
					return;
				}

				if (close(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) != -1)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Failed to input files\n");
					return;
				}

				if (outputRedirectFlag == 1)
				{

					if (strcmp(outputRedirectSymbol, "2>") == 0)
					{

						if (open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
						{
						}
						else
						{
							fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
							return;
						}

						if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDERR_FILENO) != -1)
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

						if (open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
						{
						}
						else
						{
							fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
							return;
						}

						if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDOUT_FILENO) != -1)
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

						if (open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
						{
						}
						else
						{
							fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
							return;
						}

						if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDOUT_FILENO) != -1)
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
				if (strcmp(outputRedirectSymbol, "2>") == 0)
				{
					if (open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDERR_FILENO) != -1)
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
					if (open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDOUT_FILENO) != -1)
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
					if (open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != -1)
					{
					}
					else
					{
						fprintf(stderr, "%s", "Failed to create or append to the file given as input...\n");
						return;
					}

					if (dup2(open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), STDOUT_FILENO) != -1)
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
			fprintf(stderr, "%s", "Failed fork() function!\n");
			return;
		}
		else
		{
			if (*background != 1)
			{
				setpgid(childPid, childPid);
				fgProcessPid = childPid;
				if (childPid != waitpid(childPid, NULL, WUNTRACED))
					fprintf(stderr, "%s", "Parent failed!!\n");
			}
			else
			{
				waitpid(childPid, NULL, WNOHANG);
				setpgid(childPid, childPid);
				append(&(*sPtr), childPid, args[0], 0);
				noOfProc++;
			}
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void memoryCheckUp()
{
	int i, j;
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			printf("%d ", rand() % 100);
		}
		printf("\n");
	}
	int *dynamicArray = (int *)malloc(10 * sizeof(int));
	free(dynamicArray);
	FILE *file = fopen("file.txt", "w");
	if (file != NULL)
	{
		fclose(file);
	}
}

void reqOfBmark(char *args[], pointBookmark *startPtrBookmark, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		char *tempStringComp = "\"";
		long arg2IsInt = 0;

		long i = 0;
		for (; args[i] != NULL;)
		{
			i++;
		}

		if (((strcmp(args[1], "-i") == 0) || (strcmp(args[1], "-d") == 0)) && i == 3)
		{
			char *temp = args[2];

			long index = 0;
			while (index < strlen(temp))
			{
				if (!isdigit(temp[index]))
				{
					fprintf(stderr, "%s", ERROR_CHECK_ARGUMENTS);
					arg2IsInt = 1;
				}
				index++;
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
				pointBookmark *tempPointer = startPtrBookmark;

				if (*tempPointer != NULL)
				{

					if (atoi(args[2]) != 0)
					{
						pointBookmark previousPtr = *tempPointer;
						pointBookmark tempPtr = (*tempPointer)->pointNext;
						long temp = 1;

						for (; temp != atoi(args[2]) && tempPtr != NULL;)
						{
							previousPtr = tempPtr;
							tempPtr = tempPtr->pointNext;
							temp++;
						}
						if (tempPtr != NULL)
						{
							pointBookmark delPtr = tempPtr;
							previousPtr->pointNext = tempPtr->pointNext;
							free(delPtr);
						}
						else
						{
							fprintf(stderr, "%s", "No bookmark index.\n");
						}
					}
					else
					{
						pointBookmark tempPtr = *tempPointer;
						*tempPointer = (*tempPointer)->pointNext;
						free(tempPtr);
					}
				}
				else
				{
					fprintf(stderr, "%s", EMPTY_LIST);
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
				char *progpath;

				if (*startPtrBookmark != NULL)
				{
					pointBookmark tempPtr = *startPtrBookmark;
					long j = 0;
					for (; tempPtr != NULL && j != atoi(args[2]);)
					{
						tempPtr = tempPtr->pointNext;
						j++;
					}
					if (tempPtr != NULL)
					{
						char executable[90];
						strcpy(executable, tempPtr->nameOfprog);
						long i = 0;
						executable[strlen(executable) - 2] = '\0';
						while (i < strlen(executable))
						{
							executable[i] = executable[i + 1];
							i++;
						}
						char command[100];
						sprintf(command, "%s", executable);
						system(command);
					}
					else
					{
						fprintf(stderr, "%s", "No bookmark index.\n");
					}
				}
				else
				{
					fprintf(stderr, "%s", EMPTY_LIST);
				}
				return;
			}
		}
		else if ((strcmp(args[1], "-l") == 0) && i == 2)
		{

			long count = 0;
			pointBookmark tempPointer = *startPtrBookmark;

			if (*startPtrBookmark != NULL)
			{
				for (; tempPointer->pointNext != NULL;)
				{
					printf("%ld %s\n", count, tempPointer->nameOfprog);
					count++;
					tempPointer = tempPointer->pointNext;
				}
				printf("%ld %s\n", count, tempPointer->nameOfprog);
			}
			else
			{
				fprintf(stderr, "%s", EMPTY_LIST);
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

			char command[100];
			strcpy(command, args[numOfArgs - 1]);

			if (command[strlen(args[numOfArgs - 1]) - 1] == '\"')
			{
			}
			else
			{
				fprintf(stderr, "%s", ERROR_BOOKMARK);
				return;
			}

			char *exec;
			char path[PATH_MAX + 1];
			char firstArgument[50];

			strcpy(firstArgument, args[1]);
			long t = 0;

			if (firstArgument[0] == '\"' && firstArgument[strlen(args[numOfArgs - 1]) - 1] != '\"')
			{
				t = 0;
				while (t < strlen(args[numOfArgs - 1]) - 1)
				{
					firstArgument[t] = firstArgument[t + 1];
					t++;
				}
				firstArgument[strlen(args[numOfArgs - 1]) - 1] = '\0';
			}
			else if (firstArgument[0] == '\"' && firstArgument[strlen(args[numOfArgs - 1]) - 1] == '\"')
			{
				firstArgument[strlen(args[numOfArgs - 1]) - 1] = '\0';

				t = 0;
				while (t < strlen(args[numOfArgs - 1]) - 1)
				{
					firstArgument[t] = firstArgument[t + 1];
					t++;
				}
			}
			exec = firstArgument;

			if (pathFounder(exec, path, 0))
			{
			}
			else
			{
				fprintf(stderr, "%s", "Not such a command !\n");
				return;
			}

			char executable[90] = {""};
			t = 1;
			while (t < numOfArgs)
			{
				strcat(executable, args[t]);
				strcat(executable, " ");
				t++;
			}
			pid_t tempPid;
			appendBM(startPtrBookmark, executable, 0);
			executable[0] = '\0';
		}
		else
		{
			fprintf(stderr, "%s", ERROR_BOOKMARK);
			return;
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void reqOffindPrinter(char *fileName, char *Design, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		char file[1000] = {0};

		char *buff = NULL;
		char fName[256] = {0};
		size_t len = 255;

		sprintf(fName, "grep -rnwl  %s -e %s | awk '{print $0}'", fileName, Design);
		FILE *fp = (FILE *)popen(fName, "r");

		for (; getline(&buff, &len, fp) >= 0;)
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

		sprintf(command, "grep -rnw  %s -e %s | awk '{print $0}'", fileName, Design);

		FILE *fp2 = (FILE *)popen(command, "r");
		for (; fgets(result, sizeof(result), fp2);)
		{
			strcpy(allLine, result);

			char lineNumber[15] = {0};

			long i = 0;
			long digitNum = 1;

			while (i < strlen(allLine))
			{
				if (!isdigit(allLine[i]))
				{
					break;
				}
				else
				{
					lineNumber[i] = allLine[i];
					digitNum++;
					i++;
				}
			}

			i = 0;
			while (i < strlen(allLine))
			{
				allLine[i] = allLine[digitNum + i];
				i++;
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
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void printerOfFolder(char *pathOfStart, char *Design, int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		char path[1000];
		struct dirent *dp;
		DIR *dir = opendir(pathOfStart);

		if (dir)
		{
			for (; (dp = readdir(dir)) != NULL;)
			{
				if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
				{

					char fName[50];

					strcpy(fName, dp->d_name);
					char grepFile[1000];
					strcpy(grepFile, pathOfStart);
					strcat(grepFile, "/");
					strcat(grepFile, dp->d_name);

					if (fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
															fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H'))
					{
						reqOffindPrinter(grepFile, Design, 0);
					}
					strcpy(path, pathOfStart);
					strcat(path, "/");
					strcat(path, dp->d_name);
					printerOfFolder(path, Design, 0);
				}
			}
			closedir(dir);
		}
		else
		{
			return;
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void reqOffind(char *args[], int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		long i = 0;
		for (; args[i] != NULL;)
		{
			i++;
		}

		if (i < 2 || i > 3)
		{
			fprintf(stderr, ERROR_TWO_WAYS);
			return;
		}

		if (i == 3 && strcmp(args[1], "-r") != 0)
		{
			fprintf(stderr, ERROR_CHECK_ARGUMENTS);
			return;
		}

		if (i == 2 || (i == 3 && strcmp(args[1], "-r") == 0))
		{
			char cmd[1000];
			struct dirent *de;
			DIR *dr = opendir(".");

			if (dr == NULL)
			{
				fprintf(stderr, ERROR_OPEN_DIRECTORY);
				return;
			}

			for (; (de = readdir(dr)) != NULL;)
			{
				if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
				{
					char fName[50];
					strcpy(fName, de->d_name);

					if (fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
															fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H'))
					{
						char Design[100];
						strcpy(Design, args[i - 1]);

						if (!(Design[0] == '"' && Design[strlen(args[i - 1]) - 1] == '"'))
						{
							fprintf(stderr, "Check your arguments!You need to give your Design between \" \"\n");
							closedir(dr);
							return;
						}

						reqOffindPrinter(de->d_name, args[i - 1], 0);
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
				printerOfFolder(cwd, args[2], 0);
			}
			else
			{
				fprintf(stderr, ERROR_GETCWD);
			}
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

void inputOfConfiguration(char *args[], int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		long i = 0;
		long a;
		long flag = 0;
		while (i < numOfArgs)
		{
			if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0)
			{
				args[i] = NULL;
				a = i;
				flag = 1;
				break;
			}
			i++;
		}

		if (flag != 0)
		{
			i = a + 1;
			while (i < numOfArgs)
			{
				args[i] = NULL;
				i++;
			}

			numOfArgs = numOfArgs - (numOfArgs - a);
		}
		else
		{
			return;
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

long SwitchIOControl(char *args[], int testCondition)
{
	int test = 0;
	if (test == testCondition)
	{
		if (numOfArgs == 2 && strcmp(args[0], "io") == 0 && strcmp(args[1], "-h") == 0)
		{
			printf("[1] -> \"myprog [args] > file.out\"\n");
			printf("[2] -> \"myprog [args] >> file.out\"\n");
			printf("[3] -> \"myprog [args] < file.in\"\n");
			printf("[4] -> \"myprog [args] 2> file.out\"\n");
			return 1;
		}

		long a = 0;
		long io;
		while (a < numOfArgs)
		{
			if (strcmp(args[a], "<") == 0 || strcmp(args[a], ">") == 0 || strcmp(args[a], ">>") == 0 || strcmp(args[a], "2>") == 0)
			{
				io = 1;
			}
			a++;
		}

		if (io != 1)
		{
			return 0;
		}
		else
		{
			if (strcmp(args[0], "bookmark") != 0 && strcmp(args[0], "search") != 0)
			{
			}
			else
			{
				fprintf(stderr, "Is not valid for \" %s \" command!!\n", args[0]);
				return 1;
			}
		}

		long i = 0;
		while (i < numOfArgs)
		{
			if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0)
			{
				if (i + 1 < numOfArgs)
				{
				}
				else
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
			else if (strcmp(args[i], "<") == 0 && numOfArgs > 3 && strcmp(args[i + 2], ">") == 0)
			{
				if (i + 3 < numOfArgs)
				{
				}
				else
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
			else if (strcmp(args[i], "<") == 0)
			{
				if (i + 1 < numOfArgs)
				{
				}
				else
				{
					fprintf(stderr, "%s", "Syntax error. You can type \"io -h\" to see the correct syntax.\n");
					args[0] = NULL;
					return 1;
				}

				inputRedirectFlag = 1;
				strcpy(inputFileName, args[i + 1]);

				return 0;
			}
			i++;
		}
	}
	else
	{
		printf("Test condition not satisfaied!");
	}
}

int main(void)
{

	char inputBuffer[90];
	int background;
	char *args[80];
	char path[PATH_MAX + 1];
	char *progpath;
	char *executable;

	system("clear");

	signal(SIGCHLD, SignalofCh);
	signal(SIGTSTP, pointOfSigts);

	parentPid = getpid();

	ListProcessPtr startPtr = NULL;
	pointBookmark startPtrBookmark = NULL;

	while (parentPid == getpid())
	{
		background = 0;
		if (startPtr == NULL)
			noOfProc = 1;
		printf("myshell: ");
		fflush(0);

		setup(inputBuffer, args, &background);

		if (args[0] == NULL)
			continue;
		progpath = strdup(args[0]);
		executable = args[0];

		if (SwitchIOControl(args, 0) != 0)
		{
			continue;
		}
		inputOfConfiguration(args, 0);

		if (strcmp(args[0], "exit") == 0)
		{
			ListKillofStopped(&startPtr, 0);
			if ((startPtr == NULL) != 0)
			{
				exit(1);
			}
			else
			{
				fprintf(stderr, "%s", "Background proccess running!\n");
			}
		}
		else if (strcmp(args[0], "search") == 0)
		{
			reqOffind(args, 0);
			continue;
		}
		else if (strcmp(args[0], "bookmark") == 0)
		{
			reqOfBmark(args, &startPtrBookmark, 0);
			continue;
		}
		else if (!pathFounder(executable, path, 0))
		{
			fprintf(stderr, "No executable \"%s\" found\n", executable);
			free(progpath);
		}
		else
		{
			if (*args[numOfArgs - 1] == '&')
				args[numOfArgs - 1] = '\0';
			ProcessOfconstract(path, args, &background, &startPtr, 0);
		}
		path[0] = '\0';
		inputFileName[0] = '\0';
		outputFileName[0] = '\0';
		inputRedirectFlag = 0;
		outputRedirectFlag = 0;
	}
}
