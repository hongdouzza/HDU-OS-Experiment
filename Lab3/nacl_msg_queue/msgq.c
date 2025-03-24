#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define KEY 1234
#define MAX_MSG_SIZE 1024
#define BUFFER_SIZE 256

// 定义消息结构体
struct msg_buffer {
    long msg_type;
    char msg_text[MAX_MSG_SIZE];
};

int main() {
    int msgid;
    struct msg_buffer message;
    char buffer[BUFFER_SIZE];
    
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed");
        return 1;
    } else if (pid > 0) { // 父进程：发送消息
        // 创建消息队列
        msgid = msgget(KEY, 0666 | IPC_CREAT);
        printf("msgid: %d\n", msgid);
        if (msgid == -1) {
            perror("msgget (parent)");
            exit(1);
        }
        
        printf("父进程: 消息队列已创建，请输入消息 (Ctrl+D 退出):\n");
        
        while (1) {
            // 读取用户输入
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break; // 检测到输入端关闭 (Ctrl+D)
            }
            
            // 去掉末尾的换行符
            buffer[strcspn(buffer, "\n")] = '\0';
            
            // 准备消息
            message.msg_type = 1;
            strcpy(message.msg_text, buffer);
            
            // 发送消息
            if (msgsnd(msgid, &message, strlen(message.msg_text) + 1, 0) == -1) {
                perror("msgsnd");
                break;
            }
        }
        
        // 等待子进程结束
        sleep(1);  // 给子进程一点时间接收最后的消息
        
        // 删除消息队列
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
        }
        
        // 等待子进程结束
        printf("父进程: 已结束\n");
    } else { // 子进程：接收消息
        // 等待父进程创建消息队列
        sleep(1);
        
        // 打开消息队列
        msgid = msgget(KEY, 0666);
        printf("msgid: %d\n", msgid);
        if (msgid == -1) {
            perror("msgget (child)");
            exit(1);
        }
        
        printf("子进程: 已连接到消息队列，等待接收消息...\n");
        
        while (1) {
            // 接收消息
            ssize_t bytes_read = msgrcv(msgid, &message, MAX_MSG_SIZE, 1, IPC_NOWAIT);
            if (bytes_read == -1) {
                if (errno == ENOMSG) {
                    // 没有消息可接收，等待一会再尝试
                    usleep(100000);  // 等待100ms
                    continue;
                } else if (errno == EIDRM) {
                    // 消息队列已被删除
                    break;
                }
                perror("msgrcv");
                break;
            }
            
            printf("子进程接收: %s\n", message.msg_text);
        }
        
        printf("子进程: 已结束\n");
    }
    
    return 0;
}