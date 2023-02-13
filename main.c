#include "main.h"

int main(int argc, char **argv) {
    /* 将 buffer 禁用，方便调试 */
    setbuf(stdout, NULL);

    jobInit();
    signalInit();

    char directory[UPPER_LIMITATION];
    /* 通过getcwd()函数获取当前目录 */
    getcwd(directory,UPPER_LIMITATION);
    strcat(directory, "/myshell");
    setenv("SHELL",directory,1);

    int successFlag; /* 判断是否成功的标志 */
    /* 主循环 */
    while(1) {
        // 这里可能需要初始化全局变量
        if (argc == 1) {
            pathDisplay();
            fgets(inputBuffer, UPPER_LIMITATION - 1, stdin);
            successFlag = commandTranslation();
            if (successFlag == 0)
                continue;
            else
                beforeExecution();
        }
        else if (argc == 2) { /* 说明是批处理文件输入 */
            FILE *fp;
            if((fp = fopen(argv[1], "r")) == NULL){
                fprintf(stderr, "Error: Can not open the file!\n");
                exit(1);
            }
            while (fgets(inputBuffer, UPPER_LIMITATION, fp) != NULL){
                successFlag = commandTranslation();
                if (successFlag == 0) {
                    fprintf(stderr, "Error: Wrong command\n");
                    continue;
                }
                else
                    beforeExecution();
            }
            /* 程序执行完毕，正常结束 */
            exit(0);
        }
    }
}

/*
 * 函数名: beforeExecution
 * 函数作用: 在执行命令前根据命令的情况对其进行具体执行情况的判断
 */
void beforeExecution() {
    /* 这里需要在一开始就保存起来 */
    int stdoutFd = dup(STDOUT_FILENO);
    /* 如果含有管道的操作 */
    if (isPipe) {
        int pipeFd[2]; /* 管道的文件描述符 */
        int status;
        if(pipe(pipeFd) == -1){
            fprintf(stderr, "Error: pipe() is failed!\n");
            exit(1);
        }
        /* 调用fork()函数创建子进程，返回值保存到pid1中 */
        pid_t pid1 = fork();

        /* pid1 == 0 说明是子进程*/
        if(pid1 == 0){
            /* 重定向标准输出到管道的读入端，向管道输出数据 */
            dup2(pipeFd[1], 1);
            /* 关闭管道的输出端*/
            close(pipeFd[0]);
            /* 执行该进程 */
            if (commandExecution(0,leftWordCount) == 0) {
                /* 说明是其他命令行 */
                char* leftCommand[leftWordCount + 1];
                /* 这里逐字复制左边的命令 */
                for(int i=0; i<leftWordCount; i++)
                    leftCommand[i] = commandPart[i];
                leftCommand[leftWordCount] = NULL;
                /* 调用execvp()函数执行其他的命令 */
                if(execvp(leftCommand[0],leftCommand)!=0)
                    fprintf(stderr, "Error:Wrong command!\n");
            }
            exit(0);
        }
        /* pid1 > 0 说明是主进程 */
        else if(pid1 > 0){
            /* 调用waitpid()函数使主进程等待子进程结束 */
            waitpid(pid1,&status,0);
            /* 这里需要再创建一个子进程方便执行对应的进程 */
            pid_t pid2 = fork();
            /* 创建失败，输出错误信息 */
            if(pid2 == -1)
                fprintf(stderr, "Error: fork() is failed!\n");
            /* 再子进程中 */
            else if(pid2 == 0){
                /* 关闭管道的输入端 */
                close(pipeFd[1]);
                /* 重定向标准输入到管道的输出端，从管道读取数据 */
                dup2(pipeFd[0], 0);
                /* 执行该进程 */
                /* 这里需要特别注意的是，我们在进行拆分时是没有将管道符计入的，因此这里不会有管道符 */
                if (commandExecution(leftWordCount,rightWordCount) == 0){
                    /* 说明是其他命令行 */
                    char* rightCommand[rightWordCount + 1];
                    /* 这里逐字复制右边的命令 */
                    for(int i=0; i<rightWordCount; i++)
                        rightCommand[i] = commandPart[leftWordCount + i];
                    rightCommand[rightWordCount] = NULL;
                    if(execvp(rightCommand[0], rightCommand)!=0)
                        fprintf(stderr, "Error:Wrong command!\n");
                }
                exit(0);
            }
            /* 否则说明是父进程 */
            else{
                /* 首先关闭通道 */
                close(pipeFd[0]);
                close(pipeFd[1]);
                /* 等待子进程结束 */
                waitpid(pid2,&status,0);
            }
        }
        /* 否则输出错误信息 */
        else{
            fprintf(stderr, "Error: fork() is failed!\n");
            exit(1);
        }
        return;
    }
    /* 说明含有输出重定向(覆盖) */
    if(isOutputRedirectCover){
        int fileFd = 0;
        /* 以读写，覆盖写方式打开被输出的文件 */
        fileFd = open(outputFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        /* 将标准输出重定向到相应的地方 */
        if(dup2(fileFd, STDOUT_FILENO) == -1)
            fprintf(stderr, "Error: Redirect failed!\n");
        /* 关闭文件 */
        close(fileFd);
    }
    /* 说明含有输出重定向(追加) */
    if (isOutputRedirectAdd) {
        int fileFd = 0;
        /* 以读写，覆盖写方式打开被输出的文件 */
        fileFd = open(outputFilePath, O_WRONLY | O_CREAT | O_APPEND, 0666);
        /* 将标准输出重定向到相应的地方 */
        if(dup2(fileFd, STDOUT_FILENO) == -1)
            fprintf(stderr, "Error: Redirect failed!\n");
        /* 关闭文件 */
        close(fileFd);
    }
    if (!commandExecution(0, commandWordCount)) {
        /* 说明是其他的命令行输入 */
        pid_t pid = fork();
        if(pid == 0){ /* 说明是子进程 */
            /* 进入子进程则添加环境变量PARENT来指示父进程 */
            char directory[UPPER_LIMITATION], environmentVariable[UPPER_LIMITATION];
            /* 通过getcwd()函数获取当前目录 */
            getcwd(directory,UPPER_LIMITATION);
            strcat(directory, "/myshell");
            strcpy(environmentVariable, "PARENT=");
            strcat(environmentVariable, directory);
            /* 添加新的环境变量 */
            putenv(environmentVariable);
            /* 调用execvp()执行外部命令 */
            if(execvp(commandPart[0],commandPart)!=0)
                fprintf(stderr, "Error: Wrong command named!\n");
            exit(0);
        }
        /* 否则说明是父进程 */
        else if(pid > 0){
            int status;
            if (isBg) {
                addJob(pid, commandPart[0], BG, RUNNING);
                /* 此时不需要等待子进程 */
                waitpid(pid,&status,WNOHANG);
            }
            /* 等待子进程的结束即可 */
            else {
                addJob(pid, commandPart[0], 0, 0);
                /* 此时需要等待子进程，直接返回即可 */
                waitpid(pid,&status,WUNTRACED);
            }
        }
        /* 子进程创建失败 */
        else
            fprintf(stderr, "Error: fork() is failed!\n");
    }
    /* 说明之前进行了输出重定向，此时需要复原 */
    if(isOutputRedirectCover || isOutputRedirectAdd){
        /* 复原标准输出 */
        if(dup2(stdoutFd, STDOUT_FILENO) == -1)
            fprintf(stderr,"Error: Redirect failed!\n");
        close(stdoutFd);
    }
}

/*
 * 函数名: addJob
 * 函数作用: 在自己维护的作业表中添加作业
 */
void addJob(pid_t pid, char* jobName, int type, int status) {
    int count = *jobCount;
    /* 设置相应的参数 */
    allJobs[count].pid = pid;
    allJobs[count].type = type;
    allJobs[count].status = status;
    strcpy(allJobs[count].jobName, jobName);

    *jobCount = count + 1;
    return ;
}

/*
 * 函数名: deleteJob
 * 函数作用: 在自己维护的作业表中删除对应 pid 的作业
 */
void deleteJob(pid_t pid) {
    int i;
    int count = *jobCount;
    /* 遍历 */
    for(i=0; i<count; i++){
        //找到了则退出
        if(pid == allJobs[i].pid)
            break;
    }
    /* 说明找到了 */
    if(i < count){
        for(; i<count-1; i++)
            allJobs[i] = allJobs[i+1];
        /* 注意修改 */
        *jobCount = count - 1;
    }
}

/*
 * 函数名: commandExecution
 * 函数作用: 执行命令
 */
int commandExecution(int startIndex, int length) {
    if (strcmp(commandPart[0], "exit") == 0) {
        exit(0);
    }
    else if (strcmp(commandPart[0], "cd") == 0)
        cdExecution(startIndex, length);
    else if (strcmp(commandPart[0],"clr") == 0)
        clearExecution();
    else if (strcmp(commandPart[0], "pwd") == 0)
        pwdExecution();
    else if (strcmp(commandPart[0], "time") == 0)
        timeExecution();
    else if (strcmp(commandPart[0], "echo") == 0)
        echoExecution(startIndex, length);
    else if (strcmp(commandPart[0], "dir") == 0)
        dirExecution(startIndex, length);
    else if (strcmp(commandPart[0], "umask") == 0)
        umaskExecution();
    else if (strcmp(commandPart[0], "test") == 0)
        testExecution(startIndex, length);
    else if (strcmp(commandPart[0], "set") == 0)
        setExecution();
    else if (strcmp(commandPart[0], "exec") == 0)
        execExecution(startIndex, length);
    else if (strcmp(commandPart[0], "help") == 0)
        helpExecution();
    else if (strcmp(commandPart[0], "jobs") == 0)
        jobsExecution();
    else if (strcmp(commandPart[0], "fg") == 0)
        fgExecution();
    else if (strcmp(commandPart[0], "bg") == 0)
        bgExecution();
    else
        return 0; /* return 0 表示程序还未正常结束 */
    return 1;
}

/*
 * 执行 bg 命令
 */
void bgExecution() {
    int count = *jobCount;
    int i;
    /* 遍历后台进程 */
    for(i=0; i<count; i++){
        if(allJobs[i].type == BG){
            /* 找到进程状态为挂起的进程 */
            if(allJobs[i].pid > 0 && allJobs[i].status == SUSPEND){
                allJobs[i].status = RUNNING;
                /* 使该进程继续运行 */
                kill(allJobs[i].pid, SIGCONT);
                printf("%s continue executing in background\n",allJobs[i].jobName);
                break;
            }
        }
    }
}

/*
 * 执行 fg 命令
 */
void fgExecution() {
    int count = *jobCount;
    int status;
    /* 分别代表后台进程的个数和最后一个后台进程的下标 */
    int bgCount = 0, lastBgProcess;
    /* 遍历我们自己维护的后台进程的表 */
    for(int i=0; i<count ;i++)
        if(allJobs[i].type == BG){
            bgCount++;
            lastBgProcess = i;
        }
    /* 说明没有后台或者被挂起的进程 */
    if(bgCount == 0)
        return;
    /* 否则说明有后台进程 */
    else{
        /* 先输出这个后台进程的进程名 */
        printf("%s continue executing. \n",allJobs[lastBgProcess].jobName);
        /* 如果该进程为后台挂起的进程 */
        if(allJobs[lastBgProcess].status == SUSPEND){
            /* 则改变其状态，并使主进程运行完毕 */
            allJobs[lastBgProcess].type = FG;
            allJobs[lastBgProcess].status = RUNNING;
            kill(allJobs[lastBgProcess].pid, SIGCONT);
            waitpid(allJobs[lastBgProcess].pid,&status,0);
        }
        /* 如果它是正在运行的后台进程 */
        else if(allJobs[lastBgProcess].status == RUNNING){
            /* 则改变其状态，并使主进程运行完毕 */
            allJobs[lastBgProcess].type = FG;
            waitpid(allJobs[lastBgProcess].pid,&status,0);
        }
    }
}

/*
 * 执行 jobs 命令
 */
void jobsExecution() {
    int count = *jobCount;
    /* 保存状态为 done 的进程的个数 */
    int doneCount = 0;
    int donePid[LOWER_LIMITATION];
    /* 后台进程个数 */
    int bgCount = 0;
    /* 遍历自己维护的进程表，并找出后台进程输出 */
    printf("count  pid  state		  name\n");
    for(int i=0; i<count; i++){
        if(allJobs[i].type == BG){
            if(allJobs[i].status == SUSPEND)
                printf("[%d]  %d  SUSPEND		  %s\n",bgCount+1,allJobs[i].pid,allJobs[i].jobName);
            else if(allJobs[i].status == RUNNING)
                printf("[%d]  %d  RUNNING		  %s\n",bgCount+1,allJobs[i].pid,allJobs[i].jobName);
            else{
                printf("[%d]  %d  DONE		      %s\n",bgCount+1,allJobs[i].pid,allJobs[i].jobName);
                donePid[doneCount] = allJobs[i].pid;
                doneCount++;
            }
            bgCount++;
        }

    }
    /* 删除状态为 done 的进程 */
    for(int i=0; i<doneCount; i++)
        deleteJob(donePid[i]);
}

/*
 * 执行 help 命令
 */
void helpExecution() {
    printf("----------------------------------------help----------------------------------------\n");
    printf("------------------------------------------------------------------------------------\n");
    printf("--- 一、支持的内部命令                                                           \n");
    printf("---                                                                              \n");
    printf("--- 1. bg                                                                  \n");
    printf("--- 命令作用: 将挂起的进程转为后台运行                                               \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 2. cd                                                                        \n");
    printf("--- 命令作用: 更改目录                                                              \n");
    printf("--- 参数解释: 一个参数，即为将要修改的目录                                              \n");
    printf("---                                                                              \n");
    printf("--- 3. clr                                                                       \n");
    printf("--- 命令作用: 清空屏幕的内容                                                          \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 4. dir                                                                       \n");
    printf("--- 命令作用: 显示参数目录下的文件，无参数则显示当前目录                                    \n");
    printf("--- 参数解释: 无参数或一个参数，即为要显示的目录                                          \n");
    printf("---                                                                              \n");
    printf("--- 5. echo                                                                      \n");
    printf("--- 命令作用: 显示参数的内容                                                          \n");
    printf("--- 参数解释: 任意参数                                                               \n");
    printf("---                                                                              \n");
    printf("--- 6. exec                                                                      \n");
    printf("--- 命令作用: 执行参数的命令                                                          \n");
    printf("--- 参数解释: 一个参数                                                               \n");
    printf("---                                                                              \n");
    printf("--- 7. exit                                                                      \n");
    printf("--- 命令作用: 退出shell                                                            \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 8. fg                                                                        \n");
    printf("--- 命令作用: 将挂起的进程或后台的进程转为前台运行                                        \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 9. help                                                                      \n");
    printf("--- 命令作用: 查看命令使用帮助                                                        \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 10. jobs                                                                     \n");
    printf("--- 命令作用: 显示所有后台进程                                                        \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 11. pwd                                                                      \n");
    printf("--- 命令作用: 显示当前目录的路径                                                      \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 12. set                                                                      \n");
    printf("--- 命令作用: 显示所有的环境变量                                                      \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 13. test                                                                     \n");
    printf("--- 命令作用: 进行数值测试、字符串测试和文件测试                                          \n");
    printf("--- 参数解释: 两个参数: 第一个参数是测试的条件，第二个参数是被测试的文件(字符串)                \n");
    printf("---           三个参数: 第二个参数是测试的条件，其余的参数是被测试的字符串(数字)                \n");
    printf("---                                                                              \n");
    printf("--- 14. time                                                                     \n");
    printf("--- 命令作用: 详细地显示当前的时间                                                     \n");
    printf("--- 参数解释: 无参数                                                                \n");
    printf("---                                                                              \n");
    printf("--- 15. umask                                                                    \n");
    printf("--- 命令作用: 显示当前的掩码或设置掩码                                                  \n");
    printf("--- 参数解释: 无参数或一个参数，即为设置的掩码                                            \n");
    printf("------------------------------------------------------------------------------------\n");
    printf("------------------------------------------------------------------------------------\n");
}

/*
 * 函数作用: 执行 exec 命令
 */
void execExecution(int startIndex, int length) {
    if(length == 2) {
        /* 注意exec并不创建新进程，而是用新的程序完全替换原来的程序 */
        /* 这里调用 execvp() 来实现 exec , 如果返回值不为0，说明执行出错 */
        if (execvp(commandPart[startIndex+1], &commandPart[startIndex+1]) != 0)
            fprintf(stderr, "Error: Wrong command!\n");
    }
    else{
        fprintf(stderr, "Error: Wrong parameter!\n");
    }
}

/*
 * 函数作用: 执行 set 指令，即列出所有的环境变量
 */
void setExecution() {
    /* 如果没有输入参数，则我们输出所有的环境变量 */
    if(commandWordCount == 1) {
        /* 引入变量 environ ，它存储了所有的环境变量，然后将其遍历并打印 */
        extern char ** environ;
        for(int i=0;environ[i]!=NULL;i++)
            printf("%s\n",environ[i]);
    }
    /* 说明参数的数量不对 */
    else
        fprintf(stderr, "Error: Wrong parameters!\n");
}

/*
 * 函数作用: 执行 test 指令, 执行的结果会以 TRUE 或者 FALSE 的形式显示在屏幕上
 */
void testExecution(int startIndex, int length) {
    /* 指令是否被成功执行，如果执行成功则为1 */
    int flag=0;
    if(length == 4) {

        /*
         * 字符串比较测试
         */

        /* 参数为"=", 判断两个字符串是否相等 */
        if (strcmp(commandPart[startIndex+2], "=") == 0) {
            if (strcmp(commandPart[startIndex+1], commandPart[startIndex+3]) == 0)
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"!=", 判断两个字符串是否不相等 */
        if (strcmp(commandPart[startIndex+2], "!=") == 0) {
            if (strcmp(commandPart[startIndex+1], commandPart[startIndex+3]) == 0)
                printf("FALSE\n");
            else
                printf("TRUE\n");
            flag = 1;
        }

        /*
         * 数字比较测试
         */

        /* 参数为"-eq", 判断两个数字是否相等 */
        if (strcmp(commandPart[startIndex+2], "-eq") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) == stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
        /* 参数为"-ne", 判断两个数字是否不相等 */
        if (strcmp(commandPart[startIndex+2], "-ne") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) != stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
        /* 参数为"-gt", 判断第一个数是否大于第二个数 */
        if (strcmp(commandPart[startIndex+2], "-gt") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) > stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
        /* 参数为"-ge", 判断第一个数是否大于等于第二个数 */
        if (strcmp(commandPart[startIndex+2], "-ge") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) >= stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
        /* 参数为"-lt", 判断第一个数是否小于第二个数 */
        if (strcmp(commandPart[startIndex+2], "-lt") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) < stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
        /* 参数为"-le", 判断第一个数是否小于等于第二个数 */
        if (strcmp(commandPart[startIndex+2], "-le") == 0) {
            if (isNumber(commandPart[startIndex+1]) && isNumber(commandPart[startIndex+3])){
                if (stringToNumber(commandPart[startIndex+1]) <= stringToNumber(commandPart[startIndex+3]))
                    printf("TRUE\n");
                else
                    printf("FALSE\n");
                flag = 1;
            }
            else
                fprintf(stderr, "Error: Not a number!\n");
        }
    }
    else if (length == 3) {
        /* 测试文件是否存在的结构体定义 */
        struct stat buf;

        /*
         * 字符串的长度测试
         */

        /* 参数为"-z", 判断字符串的长度是否为0 */
        if (strcmp(commandPart[startIndex+1], "-z") == 0) {
            if (strlen(commandPart[startIndex+2]) == 0)
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-n", 判断字符串的长度是否不为0 */
        if (strcmp(commandPart[startIndex+1], "-n") == 0) {
            if (strlen(commandPart[startIndex+2]) != 0)
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }

        /*
         * 文件存在测试
         */

        /* 参数为"-e", 判断文件是否存在 */
        if (strcmp(commandPart[startIndex+1], "-e") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0)
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-r", 判断文件是否存在且可读 */
        if (strcmp(commandPart[startIndex+1], "-r") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0 && access(commandPart[startIndex+2], R_OK))
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-w", 判断文件是否存在且可写 */
        if (strcmp(commandPart[startIndex+1], "-w") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0 && access(commandPart[startIndex+2], W_OK))
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-x", 判断文件是否存在且可执行 */
        if (strcmp(commandPart[startIndex+1], "-x") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0 && access(commandPart[startIndex+2], X_OK))
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-d", 判断文件是否存在且为目录 */
        if (strcmp(commandPart[startIndex+1], "-d") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0 && S_ISDIR(buf.st_mode))
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
        /* 参数为"-f", 判断文件是否存在且为普通文件 */
        if (strcmp(commandPart[startIndex+1], "-f") == 0) {
            if (lstat(commandPart[startIndex+2], &buf) == 0 && S_ISREG(buf.st_mode))
                printf("TRUE\n");
            else
                printf("FALSE\n");
            flag = 1;
        }
    }
    /* 说明指令没有被正常执行 */
    if (flag == 0)
        fprintf(stderr, "Error: Wrong parameter!\n");
}

/*
 * 函数作用: 执行 umask 命令
 */
void umaskExecution() {
    /* 没有参数，需要输出当前的掩码 */
    if(commandWordCount == 1){
        mode_t currentMask;
        /* 获得当前的掩码 */
        currentMask = umask(0);
        /* 重新设置掩码为pre_mask */
        umask(currentMask);
        printf("%04d\n",currentMask);
    }
    /* 处理错误输入 */
    else if(commandWordCount > 2 || strlen(commandPart[1]) > 4)
        fprintf(stderr, "Error: Wrong parameter!\n");
    /* 否则说明输入的参数正常，需要处理修改掩码 */
    else{
        /* 首先检查掩码是否合法 */
        int flag=0;
        /* 由于权限最高只能为 7 ，故大于 7 的单位数一定不合法 */
        for(int i = 0; i < strlen(commandPart[1]); i++){
            if(commandPart[1][i]=='8' || commandPart[1][i]=='9'){
                flag = 1; /* 说明不合法 */
                break;
            }
        }
        /* 新输入的掩码合法，进行修改 */
        if(flag == 0){
            unsigned int new_mask = atoi(commandPart[1]) % 1000;
            umask(new_mask);
        }
        /* 掩码不合法，输出错误信息 */
        else
            fprintf(stderr, "Error: Wrong parameter!\n");
    }
}

/*
 * 函数作用: 执行 dir 命令，该命令类似于 ls
 */
void dirExecution(int startIndex, int length) {
    DIR* directory;
    struct dirent * nextPtr;
    char path[UPPER_LIMITATION];
    if(length <= 2){
        /* 没有输入目录，则显示当前目录 */
        if(length == 1)
            getcwd(path,UPPER_LIMITATION);
        else /* 输入了目录 */
            strcpy(path,commandPart[startIndex+1]);
        /* 打开对应的目录 */
        directory = opendir(path);
        if (directory == NULL) {
            fprintf(stderr, "Error: Wrong directory\n");
            return ;
        }
        /* 遍历目录并将目录下的内容全部输出 */
        while((nextPtr = readdir(directory)) != NULL){
            printf("%s  ",nextPtr->d_name);
        }
        printf("\n");
        closedir(directory);
    }
    /* 否则说明参数错误 */
    else{
        fprintf(stderr, "Error: Too many parameters!\n");
    }
}

/*
 * 函数作用: 执行 echo 命令
 */
void echoExecution(int startIndex, int length) {
    for(int i = 1;i < length;i++)
        printf("%s\n",commandPart[i+startIndex]);
}

/*
 * 函数作用: 执行 time 命令
 */
void timeExecution() {
    time_t nowTime;
    struct tm *t;
    char* week[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    time(&nowTime);
    t = localtime(&nowTime);
    /* 最后需要以正确的格式输出当前的时间 */
    printf("%d/%d/%d %s %02d:%02d:%02d\n",(1900+t->tm_year),(1+t->tm_mon),t->tm_mday,week[t->tm_wday],t->tm_hour,t->tm_min,t->tm_sec);
}

/*
 * 函数作用: 执行 pwd 命令
 */
void pwdExecution() {
    char directory[UPPER_LIMITATION];
    /* 通过getcwd()函数获取当前目录 */
    getcwd(directory,UPPER_LIMITATION);
    printf("%s\n",directory);
}

/*
 * 函数作用: 执行 clr 命令，该命令类似于 clear
 */
void clearExecution() {
    system("clear");
}

/*
 * 函数作用: 执行 cd 指令
 */
void cdExecution(int startIndex, int length) {
    if (length == 1) /* 输入一个参数，说明不切换目录，直接退出即可 */
        return;
    else if (length > 2)
        fprintf(stderr, "Error: Too many parameters!\n");
    else {
        if (!chdir(commandPart[startIndex+1])) /* 需要改变环境变量 */
            setenv("PWD",commandPart[startIndex+1],1);
        else
            fprintf(stderr, "Error: Wrong directory\n");
    }
    return;
}

/*
 * 函数名: commandTranslation
 * 函数作用: 将命令进行拆分
 */
int commandTranslation() {
    int i;
    leftWordCount = 0;
    rightWordCount = 0;
    commandWordCount = 0;
    isOutputRedirectCover = 0;
    isOutputRedirectAdd = 0;
    isPipe = 0;
    isBg = 0;
    for (i = 0; i < LOWER_LIMITATION; i++)
        commandPart[i] = NULL;
    char *commandTemp;
    /* 定义分隔符 */
    char *separation = " \n\t";
    /* 取出该命令的第一个字符串 */
    commandTemp = strtok(inputBuffer, separation);
    if (commandTemp == NULL) /* 说明没有输入有效的指令 */
        return 0;
    commandPart[0] = commandTemp;
    commandWordCount++;
    while (commandPart[commandWordCount] = strtok(NULL, separation)) {
        /* isPipe为1说明支持管道操作，且以后要操作的都是后面的指令 */
        if(isPipe){
            commandWordCount++;
            rightWordCount++;
            continue; /* 以此来过滤掉命令长度变长 */
        }

        if (isOutputRedirectCover) {
            strcpy(outputFilePath, commandPart[commandWordCount]);
            continue;
        }

        if (isOutputRedirectAdd) {
            strcpy(outputFilePath, commandPart[commandWordCount]);
            continue;
        }

        /* 如果当前的分割结果为">"，说明需要进行输出重定向 */
        if (strcmp(commandPart[commandWordCount],">") == 0) {
            isOutputRedirectCover = 1;
            continue;
        }

        if (strcmp(commandPart[commandWordCount],">>") == 0) {
            isOutputRedirectAdd = 1;
            continue;
        }

        /* 如果当前分割结果为"|"，说明有管道，且注意设置左右两边的命令长度 */
        if(strcmp(commandPart[commandWordCount],"|") == 0){
            isPipe = 1;
            leftWordCount = commandWordCount;
            continue; /* 以此来过滤掉命令长度变长 */
        }
        commandWordCount++;
    }
    /* 在最后判断该程序是否需要在后台进行 */
    if(strcmp(commandPart[commandWordCount - 1],"&") == 0){
        isBg = 1;
        /* 最后一位设置为 NULL */
        commandPart[commandWordCount - 1] = NULL;
        commandWordCount--;
    }
    return 1;
}

/*
 * 函数名: pathDisplay
 * 函数作用: 显示命令行提示符
 */
void pathDisplay() {
    /* 当前的路径 */
    char path[256];
    /* 当前用户的名字 */
    char userName[64];
    /* 当前主机的名字 */
    char hostName[64];
    getcwd(path, 256);
    gethostname(hostName, 64);
    strcpy(userName, getpwuid(getuid())->pw_name);
    /* 让其以相应的颜色输出 */
    printf("\33[1;32m" "%s@%s", userName, hostName);
    printf("\33[1;37m" ":");
    printf("\33[1;34m" "%s", path);
    printf("\33[1;37m" "$ ");
}

/*
 * 函数名: jobInit
 * 函数作用: 建立一张作业表，并对作业表进行初始化
 */
void jobInit() {
    /* 创建共享内存并获取共享内存标识符 */
    int shmID = shmget((key_t)1234, sizeof(job)*MAX_JOB_NUMBER + sizeof(int), 0666 | IPC_CREAT);
    /* 链接1到当前进程的地址空间 */
    void* shm = shmat(shmID, 0, 0);
    jobShmID = shmID;
    allJobs = (job*)shm;
    jobCount = (int*)((char*)shm + sizeof(allJobs)*MAX_JOB_NUMBER);

    /* 将后台进程表全部初始化为-1 */
    for(int i=0; i<MAX_JOB_NUMBER; i++)
        allJobs[i].pid = -1;

    allJobs[0].pid = myShellPid = getpid();
    strcpy(allJobs[0].jobName, "myshell");
    allJobs[0].type = 0;
    allJobs[0].status = 0;

    *jobCount = 1;
}

/*
 * 函数名: signalInit
 * 函数作用: 对信号进行初始化
 */
void signalInit() {
    memset(&newAction, 0, sizeof(newAction));
    /* 设置new_action的信号处理函数为SIGCHLD_handler */
    newAction.sa_sigaction = SIGCHLD_handler;
    newAction.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&newAction.sa_mask);
    sigaction(SIGCHLD, &newAction, &oldAction);
    signal(SIGTSTP,SIGTSTP_handler);
    signal(SIGSTOP,SIGTSTP_handler);
}

/*
 * 函数名: SIGCHLD_handler
 */
void SIGCHLD_handler(int Signal, siginfo_t* info, void* vContext) {
    pid_t pid = info->si_pid;
    int i;
    int count = *jobCount;
    /* 遍历进程表，找到该记录 */
    for(i=0; i<count; i++){
        if(pid == allJobs[i].pid)
            break;
    }
    /* 说明找到了 */
    if(i < count){
        /* 将改作业的状态标志成已完成 */
        if(allJobs[i].type == BG && allJobs[i].status == RUNNING)
            allJobs[i].status = DONE;
        else if(allJobs[i].type == BG && allJobs[i].status == SUSPEND)
            return;
        else
            deleteJob(pid);
    }
}

/*
 * 该函数用来响应捕获到的SIGTSTP信号
 */
void SIGTSTP_handler(int Signal) {
    printf("\n");
    int i;
    int count = *jobCount;
    /* 找到此时的前台进程 */
    for(i=0; i<count; i++){
        /* 这里注意一定要判断是否与 shell 的 PID 相等，避免将 shell 终止掉 */
        if(allJobs[i].type == FG && myShellPid != allJobs[i].pid)
            break;
    }
    /* 说明找到了 */
    if(i < count){
        /* 将该进程挂起 */
        allJobs[i].status = SUSPEND;
        allJobs[i].type = BG;
        kill(allJobs[i].pid, SIGSTOP);
    }

}

/*
 * 函数名: isNumber
 * 函数作用: 判断一个数字符串是否都是数字，如果是，则返回 1 ， 否则返回 0
 */
int isNumber(char *aString) {
    int i;
    int flag=1;
    for (i=0; aString[i]!='\0'; i++){
        if (aString[i] <= '9' && aString[i] >= '0')
            continue;
        else if (aString[i] == '-' && i == 0)
            continue;
        else{
            flag=0;
            break;
        }
    }
    return flag;
}

/*
 * 函数名: stringToNumber
 * 函数作用: 提取出字符串的数字并返回
 */
int stringToNumber(char *aString) {
    int result = 0;
    int flag = 1;
    int i = 0;
    if (aString[0] == '-') {
        flag = -1;
        i++;
    }
    for ( ; aString[i] != '\0'; i++) {
        result = result * 10 + (aString[i] - '0');
    }
    return result * flag;
}