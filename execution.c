#include "main.h"

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
}

/*
 * 函数作用: 执行 clr 命令，该命令类似于 clear
 */
void clearExecution() {
    system("clear");
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
 * 函数作用: 执行 echo 命令
 */
void echoExecution(int startIndex, int length) {
    for(int i = 1;i < length;i++)
        printf("%s\n",commandPart[i+startIndex]);
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