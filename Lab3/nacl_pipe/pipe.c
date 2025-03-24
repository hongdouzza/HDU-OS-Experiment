#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>   /* 用于系统调用 */

#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1

/* 系统调用号 */
#define SYS_pipe  22
#define SYS_fork  57
#define SYS_read  0
#define SYS_write 1
#define SYS_close 3

int main() {
    char write_msg[BUFFER_SIZE];
    char read_msg[BUFFER_SIZE];
    int fd[2];
    pid_t pid;

    // 使用系统调用创建管道
    if (syscall(SYS_pipe, fd) == -1) {
        fprintf(stderr, "Pipe failed: %s\n", strerror(errno));
        return 1;
    }

    printf("fd[0]: %d, fd[1]: %d\n", fd[0], fd[1]);
    // 使用系统调用创建子进程
    pid = syscall(SYS_fork);

    if (pid < 0) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        return 1;
    }

    if (pid > 0) { // 父进程
        // 关闭读取端
        syscall(SYS_close, fd[READ_END]);

        while (1) {
            if (fgets(write_msg, BUFFER_SIZE, stdin) == NULL) {
                break;
            }
            write_msg[strcspn(write_msg, "\n")] = '\0'; // 去掉换行符

            // 使用系统调用写入消息到管道
            syscall(SYS_write, fd[WRITE_END], write_msg, strlen(write_msg) + 1);
        }

        // 关闭写入端
        syscall(SYS_close, fd[WRITE_END]);
    } else { // 子进程
        // 关闭写入端
        syscall(SYS_close, fd[WRITE_END]);

        while (1) {
            ssize_t bytes_read = syscall(SYS_read, fd[READ_END], read_msg, BUFFER_SIZE);
            if (bytes_read == 0) {
                break; // 检测到写入端关闭
            }

            printf("Child read: %s\n", read_msg);
        }

        // 关闭读取端
        syscall(SYS_close, fd[READ_END]);
    }

    return 0;
}
