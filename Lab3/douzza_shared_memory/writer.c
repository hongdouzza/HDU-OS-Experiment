#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义共享内存的键值
#define SHM_KEY 0x1234
// 定义共享内存的大小
#define BUF_SIZE 1024

int main() {
    // 创建共享内存
    int shmid = shmget(SHM_KEY, BUF_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget 失败");
        exit(EXIT_FAILURE);
    }

    // 将共享内存附加到当前进程的地址空间
    char* str = (char*)shmat(shmid, NULL, 0);
    if (str == (char*)-1) {
        perror("shmat 失败");
        shmctl(shmid, IPC_RMID, NULL);  // 清理共享内存
        exit(EXIT_FAILURE);
    }

    // 写入消息到共享内存
    const char* message = "Hello from writer process!";
    snprintf(str, BUF_SIZE, "%s", message);
    printf("写入者: 消息已写入共享内存\n");

    // 分离共享内存
    shmdt(str);

    return EXIT_SUCCESS;
}