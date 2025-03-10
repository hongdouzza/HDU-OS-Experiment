#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define QUEUE_NAME "/thread_queue"

// 在全局区域定义队列属性
struct mq_attr attr = {
    .mq_flags = 0,    // 阻塞模式
    .mq_maxmsg = 10,          // 队列最大消息数
    .mq_msgsize = 1024,       // 每条消息最大长度
    .mq_curmsgs = 0
};

void* sender(void* arg) {
    mqd_t mq = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT, 0666, &attr); // 指定属性

    if (mq == (mqd_t)-1) {
        perror("Sender: mq_open failed");
        exit(1);
    }

    char msg[] = "Hello from sender thread!";
    if (mq_send(mq, msg, strlen(msg)+1, 0) == -1) { // +1包含空终止符
        perror("Sender: mq_send failed");
    }
    mq_close(mq);
    return NULL;  // 修复缺少分号
}


void* receiver(void* arg) {
    mqd_t mq = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr); // 确保队列存在
    if (mq == (mqd_t)-1) {
        perror("Receiver: mq_open failed");
        exit(1);
    }

    char buf[1024]; // 缓冲区足够大
    ssize_t bytes_read = mq_receive(mq, buf, sizeof(buf), NULL);
    if (bytes_read == -1) {
        perror("Receiver: mq_receive failed");
    } else {
        buf[bytes_read] = '\0'; // 确保字符串终止
        printf("Received: %s\n", buf);
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME); // 删除队列（更安全的位置）
    return NULL;
}

int main() {
    // 提前创建并立即关闭队列以确保属性生效
    mqd_t pre_mq = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (pre_mq == (mqd_t)-1) {
        perror("Main: mq_open failed");
        exit(1);
    }
    mq_close(pre_mq);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, sender, NULL);
    sleep(1); // 简单同步，防止接收方先运行
    pthread_create(&t2, NULL, receiver, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
