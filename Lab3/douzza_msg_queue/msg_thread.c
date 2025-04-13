#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_PATH "/tmp"
#define PROJ_ID 'M'
#define MSG_SIZE 1024

// Message structure “09 System V message queue
struct msg_buffer {
    long mtype;       // Message type (must be > 0)
    char mtext[MSG_SIZE]; // Message data
};

void* sender(void* arg) {
    // ftok函数用于生成一个key值，key值是一个整数，用于标识一个IPC对象
    key_t key = ftok(KEY_PATH, PROJ_ID);
    if (key == -1) {
        perror("Sender: ftok failed");
        exit(1);
    }

    // msgget函数用于创建一个消息队列，如果消息队列已经存在，则返回该消息队列的标识符
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Sender: msgget failed");
        exit(1);
    }

    struct msg_buffer message;
    message.mtype = 1; // 消息类型
    strcpy(message.mtext, "Hello from sender thread!");

    // 发送消息到消息队列
    if (msgsnd(msgid, &message, strlen(message.mtext) + 1, 0) == -1) {
        perror("Sender: msgsnd failed");
    }

    return NULL;
}

void* receiver(void* arg) {
    // 生成消息队列的唯一键值
    key_t key = ftok(KEY_PATH, PROJ_ID);
    if (key == -1) {
        perror("Receiver: ftok failed");
        exit(1);
    }

    // 获取消息队列标识符
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("Receiver: msgget failed");
        exit(1);
    }

    struct msg_buffer message;

    // 接收类型为1的消息
    if (msgrcv(msgid, &message, MSG_SIZE, 1, 0) == -1) {
        perror("Receiver: msgrcv failed");
    } else {
        // 打印接收到的消息
        printf("接收到: %s\n", message.mtext);
        printf("Received: %s\n", message.mtext);
    }

    // 删除消息队列
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Receiver: msgctl failed");
    }

    return NULL;
}

int main() {
    pthread_t t1, t2;

    // 创建发送者线程
    pthread_create(&t1, NULL, sender, NULL);
    sleep(1); // 简单的同步，确保发送者线程先运行
    // 创建接收者线程
    pthread_create(&t2, NULL, receiver, NULL);

    // 等待发送者线程结束
    pthread_join(t1, NULL);
    // 等待接收者线程结束
    pthread_join(t2, NULL);

    return 0;
}
/*用户态 write()
  → SYSCALL_DEFINE3(write)
  → ksys_write()
  → vfs_write()
    → (通过 file_operations 跳转)
      → pipe_write()

fop->pipefifo_fops
//pipefifo_fops
*/