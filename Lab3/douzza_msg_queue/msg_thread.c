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

// Message structure for System V message queue
struct msg_buffer {
    long mtype;       // Message type (must be > 0)
    char mtext[MSG_SIZE]; // Message data
};

void* sender(void* arg) {
    key_t key = ftok(KEY_PATH, PROJ_ID);//ftok函数用于生成一个key值，key值是一个整数，用于标识一个IPC对象
    if (key == -1) {
        perror("Sender: ftok failed");
        exit(1);
    }

    int msgid = msgget(key, 0666 | IPC_CREAT);//msgget函数用于创建一个消息队列，如果消息队列已经存在，则返回该消息队列的标识符
    if (msgid == -1) {
        perror("Sender: msgget failed");
        exit(1);
    }

    struct msg_buffer message;
    message.mtype = 1; // Message type
    strcpy(message.mtext, "Hello from sender thread!");

    if (msgsnd(msgid, &message, strlen(message.mtext) + 1, 0) == -1) {
        perror("Sender: msgsnd failed");
    }

    return NULL;
}

void* receiver(void* arg) {
    key_t key = ftok(KEY_PATH, PROJ_ID);
    if (key == -1) {
        perror("Receiver: ftok failed");
        exit(1);
    }

    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("Receiver: msgget failed");
        exit(1);
    }

    struct msg_buffer message;

    // Receive message of type 1
    if (msgrcv(msgid, &message, MSG_SIZE, 1, 0) == -1) {
        perror("Receiver: msgrcv failed");
    } else {
        printf("Received: %s\n", message.mtext);
    }

    // Remove the message queue
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Receiver: msgctl failed");
    }

    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, sender, NULL);
    sleep(1); // Simple synchronization to ensure sender runs first
    pthread_create(&t2, NULL, receiver, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}