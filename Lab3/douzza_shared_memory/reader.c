#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SHM_KEY 0x1234
#define BUF_SIZE 1024

int main() {
    // 获取共享内存
    int shmid = shmget(SHM_KEY, BUF_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    char* str = (char*)shmat(shmid, NULL, SHM_RDONLY);
    if (str == (char*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    printf("Reader: Received data: %s\n", str);

    // 清理
    shmdt(str);
    shmctl(shmid, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}