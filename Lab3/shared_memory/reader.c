#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#define SHM_KEY 0x1234

int main() {
    int shmid = shmget(SHM_KEY, 1024, 0666);
    char* str = (char*)shmat(shmid, NULL, 0);
    printf("Read data: %s\n", str);
    shmdt(str);
    shmctl(shmid, IPC_RMID, NULL); // 销毁共享内存
    return 0;
}
