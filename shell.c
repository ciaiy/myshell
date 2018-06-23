#include "shell.h"

void initNode(CMD_NODE *cmdNode);    // 初始化控制节点
void input(char cmd[COMMAND_MAX]);  // 输入函数
void analysis_command(CMD_NODE *cmdNode);  // 将命令进行分解
void put_into_arr(char argList[ARGLIST_NUM_MAX][COMMAND_MAX], char *cmd);   // 将命令分解并放入数组中
void get_flag(char argList[ARGLIST_NUM_MAX][COMMAND_MAX], COMMAND_TYPE *flag);   // 得到命令类型
void other_work();   // 处理一些命令特有的事情
void run(CMD_NODE *cmdNode); // 按照不同类别运行命令
void shell(void); // 综合所有函数的最终体
void show(CMD_NODE *cmdNode);   // 输出控制节点的信息

void show(CMD_NODE *cmdNode) {
    printf("类型:%d\n", (int)cmdNode->type);
    printf("命令:%X\n", cmdNode->cmd);
    printf("argList\n");
    for(int i = 0; cmdNode->argList[i] != NULL; i++) {
        printf("[%d]:%s\n", i, cmdNode->argList[i]);
    }
    printf("argNext\n");
    for(int i = 0; cmdNode->argNext[i] != NULL; i++) {
        printf("[%d] : %s\n", i, cmdNode->argNext[i]);
    }
    printf("infile:%s\n", cmdNode->infile);
    printf("outfile:%s\n", cmdNode->outfile);    
}

void shell(void) {
    CMD_NODE cmdNode = {0};
    
    chdir(getenv("HOME"));  // 让shell的工作目录刚开始在用户的家目录中
    
    while(1) {
        printf("\033[;34m %s$ \033[0m", getcwd(cmdNode.workPath, FILE_PATH_MAX));
        initNode(&cmdNode);
        input(cmdNode.cmd);
        analysis_command(&cmdNode);
        other_work(&cmdNode);
        run(&cmdNode);
    }
}

void run(CMD_NODE *cmdNode) {
    pid_t pid = 0;
    
    if(cmdNode->type & IN_COMMAND || cmdNode->type & COMMAND_ERR) {
        return;
    }

    pid = fork();   // 创建子进程
    if(pid < 0) {
        printf("创建子进程失败");
        exit(-1);
    }
    
    if(pid == 0) {  // 子进程
        if(cmdNode->type & IN_REDIRECT) {   // 如果有< << 参数
            int fid;
            if(cmdNode->type & IN_REDIRECT_APP) {
                fid = open(cmdNode->infile, O_RDONLY|O_CREAT|O_APPEND, 0644);
            }else {
                fid = open(cmdNode->infile, O_RDONLY|O_CREAT, 0644);
            }            
            dup2(fid, STDIN);   // 进行输入重定向
        }

        if(cmdNode->type & OUT_REDIRECT) {   // 如果有> >> 参数
            int fod;
            if(cmdNode->type & OUT_REDIRECT_APP) {
                fod = open(cmdNode->outfile, O_WRONLY|O_CREAT|O_APPEND, 0644);
            }else {
                fod = open(cmdNode->outfile, O_WRONLY|O_CREAT, 0644);  
            }
            dup2(fod, STDOUT);   // 进行输出重定向
        }
        
        if(cmdNode->type & HAVE_PIPE) {
            pid_t pid2 = 0;
            int fd = 0;
            
            pid2 = fork();  // 再创建一个进程, 子子进程运行管道前命令, 子进程运行管道后命令
            if(pid2 < 0) {
                printf("管道命令运行失败\n");
                exit(-1);
            }
            if(pid2 == 0) { // 子子进程部分
                fd = open("/tmp/shellTemp", O_WRONLY|O_CREAT|O_TRUNC, 0644);
                dup2(fd, STDOUT);
                execvp(cmdNode->argList[0], cmdNode->argList);
            }
            if(waitpid(pid2, 0, 0) == -1) { // 子进程等待子子进程结束运行
                printf("error: 管道命令运行失败\n");
                exit(-1);
            }
            fd = open("/tmp/shellTemp", O_RDONLY);
            dup2(fd, STDIN);
            execvp(cmdNode->argNext[0], cmdNode->argNext);
            exit(0);
        }

        execvp(cmdNode->argList[0], cmdNode->argList); // 没有管道命令, 则在此运行
        printf("你输入的命令有误,请检查后重试\n");
    }
    if(cmdNode->type & BACKSTAGE) {
        printf("子进程pid为%d\n", pid);
        return ;
    }
    if(waitpid(pid, 0, 0) == -1) {
        printf("等待子进程失败\n");
    }
}

void other_work(CMD_NODE *cmdNode) {
    int argIndex = 0;
    int argListIndex = 0;
    int nextIndex = 0;
    int isNext = 0;
    int type = cmdNode->type;

    if(type & COMMAND_ERR || type & IN_COMMAND) {
        return;
    }

    while(cmdNode->arg[argIndex][0] != '\0') {

        if(type & BACKSTAGE) { // 带有后台运行命令
            cmdNode->argList[argListIndex] = NULL; // 将后台运行&屏蔽掉
            break;  // 当遇到后台运行符后, 以后的参数不进入arList中
        }
    
        if(strncmp(cmdNode->arg[argIndex], "<",1) == 0  // 判断是否为重定向输入或输出
           || strncmp(cmdNode->arg[argIndex], ">", 1) == 0) {

            argIndex++;    // 跳过> < >> << 
            if(cmdNode->arg[argIndex - 1][0] == '>') {
                strcpy(cmdNode->outfile, cmdNode->arg[argIndex]);
            }else {
                strcpy(cmdNode->infile, cmdNode->arg[argIndex]);
            }
            argIndex++;    // 跳过路径
            continue;// 跳过此次argList指向arg
        }
        
        if(strcmp(cmdNode->arg[argIndex], "|") == 0) {  // 判断是否为管道命令
            argIndex++;
            isNext = 1;
            continue;
        }

        if(isNext) {    //  如果是管道命令, 则开始给Next数组中存放数据
            cmdNode->argNext[nextIndex++] = cmdNode->arg[argIndex++];
        }else {
            cmdNode->argList[argListIndex++] = cmdNode->arg[argIndex++];
        }
    }
}

void get_flag(char arg[ARGLIST_NUM_MAX][COMMAND_MAX], COMMAND_TYPE *flag) {
    int argIndex = 0;
    int count[4] = {0};

    while(arg[argIndex][0] != '\0') {
        if(strcmp("|", arg[argIndex]) == 0) {    // 将命令的类型置为管道命令
            *flag |= HAVE_PIPE;
            count[0]++;
        }
        if(strcmp(">", arg[argIndex]) == 0) {    // 将命令的类型置为输出重定向
            *flag |= OUT_REDIRECT;
            count[1]++;
        }
        if(strcmp("<", arg[argIndex]) == 0) {    // 将命令的类型置为输入重定向
            *flag |= IN_REDIRECT;
            count[2]++;
        }
        if(strcmp("&", arg[argIndex]) == 0) {    // 将命令的类型置为后台运行
            *flag |= BACKSTAGE;
            count[3]++;                
        }
        argIndex++;
    }
    
    for(int i = 0;i < 4;i++) {
        if(count[i] > 1) { // 命令类型重复定义
            printf("error: have too many args\n");
            exit(-1);
            *flag = COMMAND_ERR;
        }   
    }
}

void put_into_arr(char arg[ARGLIST_NUM_MAX][COMMAND_MAX], char *cmd) {
    int argIndex = 0;
    int index = 0;

    while(*cmd != '\0') {
        if(*cmd == ' ') {
            argIndex++;
            index = -1;
        }else {
            arg[argIndex][index] = *cmd;
        }
        cmd++;
        index++;
    }
}

void analysis_command(CMD_NODE *cmdNode) {
    /* 自定义命令 */
    if(strcmp(cmdNode->cmd, "exit") == 0) {
        cmdNode->type = IN_COMMAND;
        exit(0);
    }
    if(strcmp(cmdNode->cmd, "pause") == 0) {
        cmdNode->type = IN_COMMAND;
        char ch;
        while((ch = getchar()) == '\r');    // 在linux中的pause和windows里的不一样
        return ;
    }
    if(strncmp(cmdNode->cmd, "cd", 2) == 0) {
        cmdNode->type = IN_COMMAND;
        put_into_arr(cmdNode->arg, cmdNode->cmd);
        chdir(cmdNode->arg[1]);
        return;        
    }
    /* 结束 */
    put_into_arr(cmdNode->arg, cmdNode->cmd); 
    get_flag(cmdNode->arg, &(cmdNode->type));
}

void input(char cmd[COMMAND_MAX]) {
    gets(cmd);
    if(strlen(cmd) >= COMMAND_MAX - 1) {
        printf("error: command too long!\n");
        exit(-1);
    }
}

void initNode(CMD_NODE *cmdNode) {
    memset(cmdNode, 0, sizeof(CMD_NODE));
    cmdNode->type = NORMAL;
}