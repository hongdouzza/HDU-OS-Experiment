#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1

int main() {
    char write_msg[BUFFER_SIZE];
    char read_msg[BUFFER_SIZE];
    int fd[2];
    pid_t pid;

    // 创建管道
    if (pipe(fd) == -1) {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    // 创建子进程
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed");
        return 1;
    }

    if (pid > 0) { // 父进程
        // 关闭读取端
        close(fd[READ_END]);

        while (1) {
            if (fgets(write_msg, BUFFER_SIZE, stdin) == NULL) {
                break; // 检测到输入端关闭
            }
            write_msg[strcspn(write_msg, "\n")] = '\0'; // 去掉换行符

            // 写入消息到管道
            write(fd[WRITE_END], write_msg, strlen(write_msg) + 1);

            if (strcmp(write_msg, "exit") == 0) {
                break;
            }
        }

        // 关闭写入端
        close(fd[WRITE_END]);
    } else { // 子进程
        // 关闭写入端
        close(fd[WRITE_END]);

        while (1) {
            ssize_t bytes_read = read(fd[READ_END], read_msg, BUFFER_SIZE);
            if (bytes_read == 0) {
                break; // 检测到写入端关闭
            }

            if (strcmp(read_msg, "exit") == 0) {
                break;
            }

            printf("Child read: %s\n", read_msg);
        }

        // 关闭读取端
        close(fd[READ_END]);
    }

    return 0;
}
