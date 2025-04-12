#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_IMAGE "myfs.bin"
#define BLOCK_SIZE 512
#define MAX_BLOCKS 2048  // 1MB存储空间
#define MAX_FILES 64
#define MAX_NAME 16

// 文件系统结构
typedef struct {
    char name[MAX_NAME];
    int start_block;
    int size;
    int is_dir;
} FileEntry;

typedef struct {
    int fat[MAX_BLOCKS];
    FileEntry entries[MAX_FILES];
    int used_blocks;
    int used_entries;
    char current_dir[MAX_NAME];
} FileSystem;

// 全局文件系统实例
static FileSystem fs;

// 初始化文件系统
void my_format() {
    memset(&fs, 0, sizeof(fs));
    fs.fat[0] = -1;  // 标记已使用
    strcpy(fs.current_dir, "/");
    printf("File system formatted.\n");
}

// 保存到磁盘
void save_disk() {
    FILE* fp = fopen(DISK_IMAGE, "wb");
    if(fp) {
        fwrite(&fs, sizeof(fs), 1, fp);
        fclose(fp);
    }
}

// 从磁盘加载
int load_disk() {
    FILE* fp = fopen(DISK_IMAGE, "rb");
    if(fp) {
        fread(&fs, sizeof(fs), 1, fp);
        fclose(fp);
        return 1;
    }
    return 0;
}

// 创建文件
void my_create(const char* name) {
    if(fs.used_entries >= MAX_FILES) {
        printf("Maximum files reached.\n");
        return;
    }
    
    FileEntry* e = &fs.entries[fs.used_entries++];
    strncpy(e->name, name, MAX_NAME);
    e->size = 0;
    e->is_dir = 0;
    printf("File '%s' created.\n", name);
    save_disk();
}

// 删除文件
void my_rm(const char* name) {
    for(int i=0; i<fs.used_entries; i++){
        if(strcmp(fs.entries[i].name, name) == 0) {
            // 释放FAT链
            int block = fs.entries[i].start_block;
            while(block != -1) {
                int next = fs.fat[block];
                fs.fat[block] = 0;
                block = next;
            }
            memset(&fs.entries[i], 0, sizeof(FileEntry));
            printf("File '%s' deleted.\n", name);
            save_disk();
            return;
        }
    }
    printf("File not found.\n");
}

// 添加目录支持
void my_mkdir(const char* name) {
    FileEntry* e = &fs.entries[fs.used_entries++];
    strncpy(e->name, name, MAX_NAME);
    e->is_dir = 1;
    save_disk();
}


// 主函数
int main() {
    if(!load_disk()) {
        printf("Creating new file system...\n");
        my_format();
    }

    char cmd[32], param[32];
    while(1) {
        printf("%s> ", fs.current_dir);
        scanf("%s", cmd);
        
        if(strcmp(cmd, "my_exitsys") == 0) break;
        if(strcmp(cmd, "my_create") == 0) {
            scanf("%s", param);
            my_create(param);
        }
        if(strcmp(cmd, "my_rm") == 0) {
            scanf("%s", param);
            my_rm(param);
        }
        if(strcmp(cmd, "my_mkdir") == 0) {
            scanf("%s", param);
            my_mkdir(param);
        }
        if(strcmp(cmd, "my_write") == 0) {
            char data[256];
            scanf("%s %s", param, data);
            my_write(param, data);
        }
    }
    
    save_disk();
    return 0;
}