#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    int pipefd[2];
    pid_t pid1, pid2;
    
    // 检查参数数量
    if (argc != 3) {
        fprintf(stderr, "用法: %s <命令1> <命令2>\n", argv[0]);
        fprintf(stderr, "示例: %s ls wc\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // 创建管道
    if (pipe(pipefd) == -1) {
        perror("管道创建失败");
        exit(EXIT_FAILURE);
    }
    
    // 创建第一个子进程
    pid1 = fork();
    
    if (pid1 < 0) {
        perror("fork失败");
        exit(EXIT_FAILURE);
    }
    
    if (pid1 == 0) { // 第一个子进程
        // 关闭管道的读端
        close(pipefd[READ_END]);
        
        // 将标准输出重定向到管道的写端
        if (dup2(pipefd[WRITE_END], STDOUT_FILENO) == -1) {
            perror("dup2失败");
            exit(EXIT_FAILURE);
        }
        
        // 关闭原来的管道写端
        close(pipefd[WRITE_END]);
        
        // 执行第一个命令
        execlp(argv[1], argv[1], NULL);
        
        // 如果execlp执行失败，会执行到这里
        perror("execlp失败");
        exit(EXIT_FAILURE);
    }
    
    // 创建第二个子进程
    pid2 = fork();
    
    if (pid2 < 0) {
        perror("fork失败");
        exit(EXIT_FAILURE);
    }
    
    if (pid2 == 0) { // 第二个子进程
        // 关闭管道的写端
        close(pipefd[WRITE_END]);
        
        // 将标准输入重定向到管道的读端
        if (dup2(pipefd[READ_END], STDIN_FILENO) == -1) {
            perror("dup2失败");
            exit(EXIT_FAILURE);
        }
        
        // 关闭原来的管道读端
        close(pipefd[READ_END]);
        
        // 执行第二个命令
        execlp(argv[2], argv[2], NULL);
        
        // 如果execlp执行失败，会执行到这里
        perror("execlp失败");
        exit(EXIT_FAILURE);
    }
    
    // 父进程关闭所有管道端
    close(pipefd[READ_END]);
    close(pipefd[WRITE_END]);
    
    // 等待两个子进程结束
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    printf("父进程: 所有子进程已结束\n");
    
    return 0;
}