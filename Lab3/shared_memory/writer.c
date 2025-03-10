#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHM_KEY 0x1234
#define BUF_SIZE 1024

int main() {
    int shmid = shmget(SHM_KEY, BUF_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    char* str = (char*)shmat(shmid, NULL, 0);
    if (str == (char*)-1) {
        perror("shmat failed");
        shmctl(shmid, IPC_RMID, NULL);  // 清理共享内存
        exit(EXIT_FAILURE);
    }

    const char* message = "Hello from writer process!";
    snprintf(str, BUF_SIZE, "%s", message);
    printf("Writer: Message written to shared memory\n");

    shmdt(str);

    return EXIT_SUCCESS;
}
