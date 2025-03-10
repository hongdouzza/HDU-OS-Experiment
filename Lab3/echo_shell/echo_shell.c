#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_LINE 80 /* 命令行的最大长度 */
#define MAX_ARGS 10 /* 最大参数个数 */
#define MAX_PATH_LEN PATH_MAX /* 路径的最大长度 */

/**
 * 函数: display_prompt
 * 描述: 显示shell提示符，包含当前工作目录
 * 返回: 无
 */
void display_prompt() {
    char cwd[MAX_PATH_LEN]; //字符数组，用来存cwd命令的输出
    
    // 获取当前工作目录
    if (getcwd(cwd, sizeof(cwd)) != NULL) { //拿到输出
        printf("$echo %s>", cwd);
    } else {
        perror("getcwd() error");
        printf("echo_shell> ");
    }
    
    fflush(stdout);
}

/**
 * 函数: read_command
 * 描述: 从标准输入读取命令
 * 参数: buffer - 存储命令的缓冲区
 * 返回: 无
 */
void read_command(char *buffer) {
    // 显示带有当前目录的提示符
    display_prompt(); //展示prompt
    
    if (fgets(buffer, MAX_LINE, stdin) == NULL) {//什么都没有
        printf("\n");
        exit(0); 
    }
    
    // 去除换行符
    size_t length = strlen(buffer); 
    if (buffer[length - 1] == '\n') {// 检查读取到的字符串是否以换行符（\n）结尾。因为 fgets() 会将换行符也读取到字符串中，如果存在换行符，就将其替换为字符串结束符 \0，去掉换行符。
        buffer[length - 1] = '\0';
    }
}

/**
 * 函数: parse_command
 * 描述: 解析命令行为参数数组
 * 参数: buffer - 命令字符串
 *       args - 参数数组
 * 返回: 参数个数
 */
int parse_command(char *buffer, char *args[]) {
    int count = 0;
    char *token;
    
    token = strtok(buffer, " \t"); //根据空格和Tab分割命令
    while (token != NULL && count < MAX_ARGS) {
        args[count++] = token;
        token = strtok(NULL, " \t");
    }
    args[count] = NULL; // 参数数组以NULL结尾
    
    return count;
}

/**
 * 函数: execute_command
 * 描述: 执行命令
 * 参数: args - 参数数组
 * 返回: 无
 */
void execute_command(char *args[]) {
    pid_t pid;
    int status;
    
    // 检查内建命令
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "echo_shell: expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("echo_shell: cd");
            }
        }
        return;
    }
    
    // 创建子进程执行命令
    pid = fork();
    
    if (pid < 0) {
        // fork 失败
        perror("echo_shell: fork failed");
    } else if (pid == 0) {
        // 子进程
        if (execvp(args[0], args) == -1) {
            perror("echo_shell: exec failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // 父进程
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

/**
 * 主函数: main
 * 描述: shell主循环
 */
int main(void) {
    char buffer[MAX_LINE];
    char *args[MAX_ARGS + 1]; // +1 用来存Null值代表已经处理完成
    
    printf("echo_Shell 启动. 输入 'exit' 退出.\n");
    
    while (1) {
        // 读取命令
        read_command(buffer);
        
        // 解析命令
        if (parse_command(buffer, args) > 0) {
            // 执行命令
            execute_command(args);
        }
    }
    //跑路
    return 0;
}