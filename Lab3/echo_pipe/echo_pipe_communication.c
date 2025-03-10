#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUF 1024

int main() {
    int pipefd[2]; //新建两个管道文件描述符
    pid_t pid; //定义pid
    char write_msg[] = "Hello from the child process!";//来自子进程的欢迎语句
    char read_msg[MAX_BUF];

    if (pipe(pipefd) == -1) { //创建失败，报错
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();//创建子进程

    if (pid == -1) { //fork失败，报错
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {  // 父进程
        close(pipefd[1]);  // 关闭写端
        read(pipefd[0], read_msg, sizeof(read_msg)); //读数据
        printf("Parent received: %s\n", read_msg); //打印语句
        close(pipefd[0]); //关闭读端
    } else {  // 子进程
        close(pipefd[0]);  // 关闭读端
        write(pipefd[1], write_msg, strlen(write_msg) + 1);  // 向管道写数据
        close(pipefd[1]);  //关闭写端
        exit(0);  //跑路
    }

    return 0;
}
