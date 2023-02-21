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