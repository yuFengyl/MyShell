#include "main.h"

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