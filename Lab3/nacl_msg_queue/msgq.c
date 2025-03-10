#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define QUEUE_NAME "/msg_queue"
#define MAX_MSG_SIZE 1024
#define BUFFER_SIZE 256

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char buffer[BUFFER_SIZE];
    
    // 设置消息队列属性
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed");
        return 1;
    } else if (pid > 0) { // 父进程：发送消息
        mq_unlink(QUEUE_NAME); // 删除可能已存在的消息队列
        mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0666, &attr); // 创建消息队列
        printf("mq: %d\n", mq);
        if (mq == (mqd_t)-1) {
            perror("mq_open (parent)");
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
            
            // 发送消息
            if (mq_send(mq, buffer, strlen(buffer) + 1, 0) == -1) {
                perror("mq_send");
                break;
            }
        }
        
        // 等待子进程结束
        sleep(1);  // 给子进程一点时间接收最后的消息
        
        // 关闭并删除消息队列
        mq_close(mq);
        mq_unlink(QUEUE_NAME);
        
        // 等待子进程结束
        printf("父进程: 已结束\n");
    } else { // 子进程：接收消息
        // 等待父进程创建消息队列
        sleep(1);
        
        // 打开消息队列
        mq = mq_open(QUEUE_NAME, O_RDONLY);
        printf("mq: %d\n", mq);
        if (mq == (mqd_t)-1) {
            perror("mq_open (child)");
            exit(1);
        }
        
        printf("子进程: 已连接到消息队列，等待接收消息...\n");
        
        while (1) {
            ssize_t bytes_read;
            
            // 接收消息
            bytes_read = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EINTR) {
                    continue;  // 临时错误，重试
                } else if (errno == EINVAL) {
                    // 消息队列可能已被删除
                    break;
                }
                perror("mq_receive");
                break;
            }
            
            printf("子进程接收: %s\n", buffer);
        }
        
        // 关闭消息队列
        mq_close(mq);
        printf("子进程: 已结束\n");
    }
    
    return 0;
}