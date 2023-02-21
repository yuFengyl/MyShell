#include "main.h"
/*
 * 函数名: beforeExecution
 * 函数作用: 在执行命令前根据命令的情况对其进行具体执行情况的判断
 */
void beforeExecution() {
    /* 这里需要在一开始就将标准输出保存起来，用于后续的复原 */
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
            /* 重定向标准输出到管道的读入端，向管道中写入数据 */
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
        /* pid1 > 0 说明是父进程 */
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