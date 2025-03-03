#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#define QUEUE_NAME "/thread_queue"

void* sender(void* arg) {
    mqd_t mq = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT, 0666, NULL);
    char msg[] = "Hello from sender thread!";
    mq_send(mq, msg, sizeof(msg), 0);
    mq_close(mq);
    return NULL;
}

void* receiver(void* arg) {
    mqd_t mq = mq_open(QUEUE_NAME, O_RDONLY);
    char buf;
    mq_receive(mq, buf, sizeof(buf), NULL);
    printf("Received: %s\n", buf);
    mq_close(mq);
    mq_unlink(QUEUE_NAME); // 删除队列
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, sender, NULL);
    pthread_create(&t2, NULL, receiver, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
