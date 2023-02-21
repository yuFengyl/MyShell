#ifndef MYSHELL_MAIN_H
#define MYSHELL_MAIN_H

#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<signal.h>
#include<pwd.h>
#include<time.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/shm.h>

/* 宏定义 */
#define UPPER_LIMITATION 256
#define MAX_JOB_NUMBER 64
#define LOWER_LIMITATION 16
/* 关于作业状态的定义 */
#define FG 0
#define BG 1
#define RUNNING 0
#define SUSPEND 1
#define DONE 2

/* 函数定义 */
void pathDisplay();
int commandTranslation();
int commandExecution(int startIndex, int length);
void beforeExecution();
void jobInit();
void signalInit();
void SIGCHLD_handler(int Signal, siginfo_t* info, void* vContext);
void SIGTSTP_handler(int Signal);
int isNumber(char *aString);
int stringToNumber(char *aString);

void bgExecution();
void fgExecution();
void cdExecution(int startIndex, int length);
void clearExecution();
void pwdExecution();
void timeExecution();
void echoExecution(int startIndex, int length);
void dirExecution(int startIndex, int length);
void umaskExecution();
void testExecution(int startIndex, int length);
void setExecution();
void execExecution(int startIndex, int length);
void helpExecution();
void jobsExecution();
void addJob(pid_t pid, char* jobName, int type, int status);
void deleteJob(pid_t pid);

/* 变量定义 */
char inputBuffer[UPPER_LIMITATION]; /* 输入的指令 */
char *commandPart[LOWER_LIMITATION]; /* 输入的指令被拆分成很多个部分 */
int commandWordCount; /* 指令单独的词的数量 */
int isPipe; /* 以此来判断指令中是否包含管道操作, 为 1 说明有管道，否则说明没有管道 */
int leftWordCount; /* 指令中左边指令的词的数量 */
int rightWordCount; /* 指令中右边指令的词的数量 */
int isOutputRedirectCover; /* 输出重定向覆盖标志，1 代表重定向，否则说明没有 */
int isOutputRedirectAdd; /* 输出重定向追加标志，1 代表重定向，否则说明没有 */
char outputFilePath[UPPER_LIMITATION]; /* 输出重定向的路径名 */
pid_t myShellPid;
int *jobCount;
int isBg; /* 用来判断该程序是否需要在后台执行 */

/* 结构体定义 */
typedef struct jobInformation{
    pid_t pid;
    char jobName[UPPER_LIMITATION];
    int type; /* 0表示fg，1表示bg */
    int status; /* 0表示正在运行 */
} job;

job* allJobs;
struct sigaction oldAction;
struct sigaction newAction;

#endif //MYSHELL_MAIN_H
