#include "main.h"

/*
 * 函数名: jobInit
 * 函数作用: 建立一张作业表，并对作业表进行初始化
 */
void jobInit() {
    /* 创建共享内存并获取共享内存标识符 */
    int shmID = shmget((key_t)1234, sizeof(job)*MAX_JOB_NUMBER + sizeof(int), 0666 | IPC_CREAT);
    /* 链接1到当前进程的地址空间 */
    void* shm = shmat(shmID, 0, 0);
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