#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// 定义共享内存的键值
#define SHM_KEY 0x1234
// 定义共享内存的大小
#define BUF_SIZE 1024

int main() {
    // 获取共享内存
    int shmid = shmget(SHM_KEY, BUF_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget 失败");
        exit(EXIT_FAILURE);
    }

    // 将共享内存附加到当前进程的地址空间
    char* str = (char*)shmat(shmid, NULL, SHM_RDONLY);
    if (str == (char*)-1) {
        perror("shmat 失败");
        exit(EXIT_FAILURE);
    }

    // 打印接收到的数据
    printf("读取者: 接收到的数据: %s\n", str);

    // 分离共享内存
    shmdt(str);
    // 删除共享内存
    shmctl(shmid, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}