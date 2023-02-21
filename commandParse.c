#include "main.h"

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
    while ((commandPart[commandWordCount] = strtok(NULL, separation))) {
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