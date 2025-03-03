#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#define SHM_KEY 0x1234

int main() {
    int shmid = shmget(SHM_KEY, 1024, 0666|IPC_CREAT);
    char* str = (char*)shmat(shmid, NULL, 0);
    sprintf(str, "Hello from writer process!");
    shmdt(str);
    return 0;
}
