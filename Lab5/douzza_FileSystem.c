/*
 * 简单文件系统实现
 *
 * 本实现在内存中创建一个虚拟磁盘，并实现一个具有多级目录结构的简单文件系统。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

/* 常量定义 */
#define BLOCK_SIZE 1024       // 每个块的大小（字节）
#define DISK_SIZE 1024*1024   // 虚拟磁盘总大小（1MB）
#define BLOCK_NUM (DISK_SIZE / BLOCK_SIZE)  // 块的数量
#define MAX_FILENAME_LENGTH 28  // 最大文件名长度
#define MAX_OPEN_FILES 16     // 同时打开的最大文件数
#define MAX_PATH_LENGTH 256   // 最大路径长度
#define ROOT_BLOCK 0          // 根目录从块0开始
#define FAT_BLOCK 1           // FAT表从块1开始
#define DATA_BLOCK 2          // 数据块从块2开始
#define EOF_BLOCK 0xFFFF      // FAT中的文件结束标记

/* 结构体定义 */

// FAT表条目
typedef unsigned short FAT_ENTRY;

// 文件/目录属性
typedef struct {
    unsigned char is_dir : 1;     // 是否是目录
    unsigned char read : 1;       // 读权限
    unsigned char write : 1;      // 写权限
    unsigned char reserved : 5;   // 保留位
} Attributes;

// 目录项结构
typedef struct {
    char filename[MAX_FILENAME_LENGTH];  // 文件名
    Attributes attr;                     // 文件属性
    unsigned short first_block;          // 第一个数据块号
    unsigned int file_size;              // 文件大小
    time_t create_time;                  // 创建时间
} DirEntry;

// 打开文件表项
typedef struct {
    char filename[MAX_FILENAME_LENGTH];  // 文件名
    unsigned short first_block;          // 第一个数据块号
    unsigned int file_size;              // 文件大小
    unsigned int current_pos;            // 当前位置
    bool is_used;                        // 是否使用
    bool can_read;                       // 是否可读
    bool can_write;                      // 是否可写
} OpenFileEntry;

/* 全局变量 */
unsigned char* virtual_disk = NULL;        // 虚拟磁盘
FAT_ENTRY* fat = NULL;                     // 指向FAT表的指针
OpenFileEntry open_file_table[MAX_OPEN_FILES];  // 打开文件表
char current_dir[MAX_PATH_LENGTH] = "/";   // 当前目录
unsigned short current_dir_block = ROOT_BLOCK; // 当前目录块

/* 函数声明 */
void my_format();
int my_mkdir(const char* dirname);
int my_rmdir(const char* dirname);
void my_ls();
int my_cd(const char* dirname);
int my_create(const char* filename);
int my_open(const char* filename, char mode);
int my_close(int fd);
int my_write(int fd, const char* buffer, int length);
int my_read(int fd, char* buffer, int length);
int my_rm(const char* filename);
void my_exitsys();

// 辅助函数
unsigned short alloc_block();
void free_block(unsigned short block);
int find_file_or_dir(const char* name, DirEntry* entry);
int find_empty_entry();
void save_to_file(const char* filename);
void load_from_file(const char* filename);

/* 文件系统实现 */

// 格式化虚拟磁盘
void my_format() {
    printf("格式化文件系统...\n");

    // 释放之前的虚拟磁盘（如果存在）
    if (virtual_disk != NULL) {
        free(virtual_disk);
    }

    // 分配虚拟磁盘空间
    virtual_disk = (unsigned char*)malloc(DISK_SIZE);
    if (virtual_disk == NULL) {
        printf("内存分配失败！\n");
        exit(1);
    }

    // 初始化所有块为0
    memset(virtual_disk, 0, DISK_SIZE);

    // 初始化FAT表
    fat = (FAT_ENTRY*)(virtual_disk + FAT_BLOCK * BLOCK_SIZE);

    // 设置已使用的块（根目录和FAT表）
    fat[ROOT_BLOCK] = EOF_BLOCK;
    fat[FAT_BLOCK] = EOF_BLOCK;

    // 将其余块标记为空闲
    for (int i = DATA_BLOCK; i < BLOCK_NUM; i++) {
        fat[i] = 0;
    }

    // 初始化根目录
    DirEntry* root_dir = (DirEntry*)(virtual_disk + ROOT_BLOCK * BLOCK_SIZE);
    memset(root_dir, 0, BLOCK_SIZE);

    // 设置当前目录为根目录
    strcpy(current_dir, "/");
    current_dir_block = ROOT_BLOCK;

    // 初始化打开文件表
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_file_table[i].is_used = false;
    }

    printf("文件系统格式化完成！\n");
}

// 创建目录
int my_mkdir(const char* dirname) {
    if (strlen(dirname) >= MAX_FILENAME_LENGTH) {
        printf("目录名过长！\n");
        return -1;
    }

    // 检查目录是否已存在
    DirEntry temp;
    if (find_file_or_dir(dirname, &temp) != -1) {
        printf("目录 %s 已存在！\n", dirname);
        return -1;
    }

    // 查找当前目录中的空条目
    DirEntry* current_dir_entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
    int empty_entry = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
        if (current_dir_entries[i].filename[0] == '\0') {
            empty_entry = i;
            break;
        }
    }

    if (empty_entry == -1) {
        printf("当前目录已满！\n");
        return -1;
    }

    // 分配新块用于目录
    unsigned short new_block = alloc_block();
    if (new_block == 0) {
        printf("磁盘空间不足！\n");
        return -1;
    }

    // 初始化新目录块
    memset(virtual_disk + new_block * BLOCK_SIZE, 0, BLOCK_SIZE);

    // 在当前目录中创建新条目
    strcpy(current_dir_entries[empty_entry].filename, dirname);
    current_dir_entries[empty_entry].attr.is_dir = 1;
    current_dir_entries[empty_entry].attr.read = 1;
    current_dir_entries[empty_entry].attr.write = 1;
    current_dir_entries[empty_entry].first_block = new_block;
    current_dir_entries[empty_entry].file_size = 0;
    current_dir_entries[empty_entry].create_time = time(NULL);

    printf("目录 %s 创建成功！\n", dirname);
    return 0;
}

// 删除目录
int my_rmdir(const char* dirname) {
    // 不允许删除"."和".."
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        printf("不能删除 %s 目录！\n", dirname);
        return -1;
    }

    // 查找目录
    DirEntry entry;
    int entry_index = find_file_or_dir(dirname, &entry);
    if (entry_index == -1) {
        printf("目录 %s 不存在！\n", dirname);
        return -1;
    }

    // 确保是目录
    if (!entry.attr.is_dir) {
        printf("%s 不是目录！\n", dirname);
        return -1;
    }

    // 检查目录是否为空
    DirEntry* dir_entries = (DirEntry*)(virtual_disk + entry.first_block * BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
        if (dir_entries[i].filename[0] != '\0') {
            printf("目录 %s 不为空！\n", dirname);
            return -1;
        }
    }

    // 释放目录占用的块
    free_block(entry.first_block);

    // 从当前目录中删除条目
    DirEntry* current_dir_entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
    memset(&current_dir_entries[entry_index], 0, sizeof(DirEntry));

    printf("目录 %s 删除成功！\n", dirname);
    return 0;
}

// 显示当前目录内容
void my_ls() {
    DirEntry* entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);

    printf("当前目录: %s\n", current_dir);
    printf("名称\t\t\t类型\t大小\t创建时间\t权限\n");

    for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
        if (entries[i].filename[0] != '\0') {
            char type = entries[i].attr.is_dir ? 'd' : 'f';
            char perm[4] = "---";
            if (entries[i].attr.read) perm[0] = 'r';
            if (entries[i].attr.write) perm[1] = 'w';

            char time_str[30];
            struct tm* timeinfo = localtime(&entries[i].create_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

            printf("%-20s\t%c\t%5d\t%s\t%s\n",
                   entries[i].filename,
                   type,
                   entries[i].file_size,
                   time_str,
                   perm);
        }
    }
}

// 切换目录
int my_cd(const char* dirname) {
    // 处理特殊情况
    if (strcmp(dirname, ".") == 0) {
        // 保持在当前目录
        return 0;
    }

    if (strcmp(dirname, "..") == 0) {
        // 返回上级目录
        if (strcmp(current_dir, "/") == 0) {
            // 已经在根目录
            return 0;
        }

        // 查找最后一个"/"的位置
        char* last_slash = strrchr(current_dir, '/');
        if (last_slash == current_dir) {
            // 根目录的情况
            current_dir[1] = '\0';
            current_dir_block = ROOT_BLOCK;
        } else {
            *last_slash = '\0';

            // 查找新的当前目录对应的块
            char* dir_name = last_slash + 1;
            if (dir_name[0] == '\0') {
                // 处理路径结尾的/
                last_slash = strrchr(current_dir, '/');
                if (last_slash == current_dir) {
                    current_dir[1] = '\0';
                    current_dir_block = ROOT_BLOCK;
                } else {
                    *last_slash = '\0';
                    dir_name = last_slash + 1;

                    // 查找上级目录
                    // 这里实际实现需要更完善，暂时简化处理
                    current_dir_block = ROOT_BLOCK;
                }
            }
        }

        return 0;
    }

    if (strcmp(dirname, "/") == 0) {
        // 切换到根目录
        strcpy(current_dir, "/");
        current_dir_block = ROOT_BLOCK;
        return 0;
    }

    // 查找目录
    DirEntry entry;
    int entry_index = find_file_or_dir(dirname, &entry);
    if (entry_index == -1) {
        printf("目录 %s 不存在！\n", dirname);
        return -1;
    }

    // 确保是目录
    if (!entry.attr.is_dir) {
        printf("%s 不是目录！\n", dirname);
        return -1;
    }

    // 更新当前目录
    if (strcmp(current_dir, "/") != 0) {
        strcat(current_dir, "/");
    }
    strcat(current_dir, dirname);
    current_dir_block = entry.first_block;

    return 0;
}

// 创建文件
int my_create(const char* filename) {
    if (strlen(filename) >= MAX_FILENAME_LENGTH) {
        printf("文件名过长！\n");
        return -1;
    }

    // 检查文件是否已存在
    DirEntry temp;
    if (find_file_or_dir(filename, &temp) != -1) {
        printf("文件 %s 已存在！\n", filename);
        return -1;
    }

    // 查找当前目录中的空条目
    DirEntry* current_dir_entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
    int empty_entry = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
        if (current_dir_entries[i].filename[0] == '\0') {
            empty_entry = i;
            break;
        }
    }

    if (empty_entry == -1) {
        printf("当前目录已满！\n");
        return -1;
    }

    // 分配新块用于文件
    unsigned short new_block = alloc_block();
    if (new_block == 0) {
        printf("磁盘空间不足！\n");
        return -1;
    }

    // 在当前目录中创建新条目
    strcpy(current_dir_entries[empty_entry].filename, filename);
    current_dir_entries[empty_entry].attr.is_dir = 0; // 文件而非目录
    current_dir_entries[empty_entry].attr.read = 1;
    current_dir_entries[empty_entry].attr.write = 1;
    current_dir_entries[empty_entry].first_block = new_block;
    current_dir_entries[empty_entry].file_size = 0;
    current_dir_entries[empty_entry].create_time = time(NULL);

    printf("文件 %s 创建成功！\n", filename);
    return 0;
}

// 打开文件
int my_open(const char* filename, char mode) {
    // 查找文件
    DirEntry entry;
    int entry_index = find_file_or_dir(filename, &entry);
    if (entry_index == -1) {
        printf("文件 %s 不存在！\n", filename);
        return -1;
    }

    // 确保是文件而非目录
    if (entry.attr.is_dir) {
        printf("%s 是目录而非文件！\n", filename);
        return -1;
    }

    // 检查权限
    if (mode == 'r' && !entry.attr.read) {
        printf("没有读取权限！\n");
        return -1;
    }

    if (mode == 'w' && !entry.attr.write) {
        printf("没有写入权限！\n");
        return -1;
    }

    // 在打开文件表中查找空闲项
    int fd = find_empty_entry();
    if (fd == -1) {
        printf("打开文件数已达上限！\n");
        return -1;
    }

    // 填充打开文件表项
    strcpy(open_file_table[fd].filename, filename);
    open_file_table[fd].first_block = entry.first_block;
    open_file_table[fd].file_size = entry.file_size;
    open_file_table[fd].current_pos = 0;
    open_file_table[fd].is_used = true;
    open_file_table[fd].can_read = (mode == 'r' || mode == 'a');
    open_file_table[fd].can_write = (mode == 'w' || mode == 'a');

    // 如果是写模式，清空文件内容
    if (mode == 'w') {
        // 释放可能存在的链接块
        unsigned short block = entry.first_block;
        unsigned short next_block;

        while (block != EOF_BLOCK) {
            next_block = fat[block];
            if (block != entry.first_block) {
                fat[block] = 0; // 标记为空闲
            }
            block = next_block;
        }

        // 更新目录项
        DirEntry* current_dir_entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
        current_dir_entries[entry_index].file_size = 0;

        // 清空文件首块
        memset(virtual_disk + entry.first_block * BLOCK_SIZE, 0, BLOCK_SIZE);
        fat[entry.first_block] = EOF_BLOCK;
    }

    // 如果是追加模式，将位置设在文件末尾
    if (mode == 'a') {
        open_file_table[fd].current_pos = entry.file_size;
    }

    printf("文件 %s 打开成功，文件描述符为 %d\n", filename, fd);
    return fd;
}

// 关闭文件
int my_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_file_table[fd].is_used) {
        printf("无效的文件描述符！\n");
        return -1;
    }

    // 清除打开文件表项
    open_file_table[fd].is_used = false;

    printf("文件描述符 %d 关闭成功！\n", fd);
    return 0;
}

// 写文件
int my_write(int fd, const char* buffer, int length) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_file_table[fd].is_used) {
        printf("无效的文件描述符！\n");
        return -1;
    }

    if (!open_file_table[fd].can_write) {
        printf("文件没有写入权限！\n");
        return -1;
    }

    int bytes_written = 0;
    int current_pos = open_file_table[fd].current_pos;
    unsigned short current_block = open_file_table[fd].first_block;

    // 找到对应的数据块
    int block_offset = current_pos / BLOCK_SIZE;
    for (int i = 0; i < block_offset; i++) {
        current_block = fat[current_block];
        if (current_block == EOF_BLOCK) {
            // 需要分配新块
            unsigned short new_block = alloc_block();
            if (new_block == 0) {
                printf("磁盘空间不足！\n");
                break;
            }

            // 更新FAT
            fat[current_block] = new_block;
            fat[new_block] = EOF_BLOCK;
            current_block = new_block;
        }
    }

    // 写入数据
    while (bytes_written < length) {
        // 计算当前块内偏移
        int offset_in_block = current_pos % BLOCK_SIZE;

        // 计算当前块可写入的字节数
        int bytes_to_write = BLOCK_SIZE - offset_in_block;
        if (bytes_to_write > length - bytes_written) {
            bytes_to_write = length - bytes_written;
        }

        // 写入数据
        memcpy(virtual_disk + current_block * BLOCK_SIZE + offset_in_block,
               buffer + bytes_written,
               bytes_to_write);

        bytes_written += bytes_to_write;
        current_pos += bytes_to_write;

        // 检查是否需要分配新块
        if (bytes_written < length && offset_in_block + bytes_to_write >= BLOCK_SIZE) {
            if (fat[current_block] == EOF_BLOCK) {
                // 需要分配新块
                unsigned short new_block = alloc_block();
                if (new_block == 0) {
                    printf("磁盘空间不足！\n");
                    break;
                }

                // 更新FAT
                fat[current_block] = new_block;
                fat[new_block] = EOF_BLOCK;
            }

            current_block = fat[current_block];
        }
    }

    // 更新文件大小
    if (current_pos > open_file_table[fd].file_size) {
        open_file_table[fd].file_size = current_pos;

        // 更新目录项中的文件大小
        DirEntry* entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
        for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
            if (entries[i].filename[0] != '\0' &&
                strcmp(entries[i].filename, open_file_table[fd].filename) == 0) {
                entries[i].file_size = current_pos;
                break;
            }
        }
    }

    // 更新当前位置
    open_file_table[fd].current_pos = current_pos;

    return bytes_written;
}

// 读文件
int my_read(int fd, char* buffer, int length) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_file_table[fd].is_used) {
        printf("无效的文件描述符！\n");
        return -1;
    }

    if (!open_file_table[fd].can_read) {
        printf("文件没有读取权限！\n");
        return -1;
    }

    int bytes_read = 0;
    int current_pos = open_file_table[fd].current_pos;
    unsigned short current_block = open_file_table[fd].first_block;

    // 如果已经到文件末尾，直接返回0
    if (current_pos >= open_file_table[fd].file_size) {
        return 0;
    }

    // 限制读取长度不超过文件大小
    if (current_pos + length > open_file_table[fd].file_size) {
        length = open_file_table[fd].file_size - current_pos;
    }

    // 找到对应的数据块
    int block_offset = current_pos / BLOCK_SIZE;
    for (int i = 0; i < block_offset; i++) {
        current_block = fat[current_block];
        if (current_block == EOF_BLOCK) {
            // 文件结构损坏
            printf("文件结构损坏！\n");
            return -1;
        }
    }

    // 读取数据
    while (bytes_read < length) {
        // 计算当前块内偏移
        int offset_in_block = current_pos % BLOCK_SIZE;

        // 计算当前块可读取的字节数
        int bytes_to_read = BLOCK_SIZE - offset_in_block;
        if (bytes_to_read > length - bytes_read) {
            bytes_to_read = length - bytes_read;
        }

        // 读取数据
        memcpy(buffer + bytes_read,
               virtual_disk + current_block * BLOCK_SIZE + offset_in_block,
               bytes_to_read);

        bytes_read += bytes_to_read;
        current_pos += bytes_to_read;

        // 检查是否需要继续读下一块
        if (bytes_read < length && offset_in_block + bytes_to_read >= BLOCK_SIZE) {
            current_block = fat[current_block];
            if (current_block == EOF_BLOCK) {
                // 已到文件末尾
                break;
            }
        }
    }

    // 更新当前位置
    open_file_table[fd].current_pos = current_pos;

    return bytes_read;
}

// 删除文件
int my_rm(const char* filename) {
    // 查找文件
    DirEntry entry;
    int entry_index = find_file_or_dir(filename, &entry);
    if (entry_index == -1) {
        printf("文件 %s 不存在！\n", filename);
        return -1;
    }

    // 确保是文件而非目录
    if (entry.attr.is_dir) {
        printf("%s 是目录而非文件！\n", filename);
        return -1;
    }

    // 检查文件是否已打开
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_file_table[i].is_used &&
            strcmp(open_file_table[i].filename, filename) == 0) {
            printf("文件 %s 已打开，不能删除！\n", filename);
            return -1;
        }
    }

    // 释放文件占用的所有块
    unsigned short block = entry.first_block;
    unsigned short next_block;

    while (block != EOF_BLOCK) {
        next_block = fat[block];
        fat[block] = 0; // 标记为空闲
        block = next_block;
    }

    // 从目录中删除条目
    DirEntry* current_dir_entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);
    memset(&current_dir_entries[entry_index], 0, sizeof(DirEntry));

    printf("文件 %s 删除成功！\n", filename);
    return 0;
}

// 退出文件系统
void my_exitsys() {
    // 保存文件系统状态
    save_to_file("filesystem.img");

    // 释放虚拟磁盘
    if (virtual_disk != NULL) {
        free(virtual_disk);
        virtual_disk = NULL;
    }

    printf("文件系统已安全退出！\n");
}

/* 辅助函数实现 */

// 分配一个空闲块
unsigned short alloc_block() {
    for (int i = DATA_BLOCK; i < BLOCK_NUM; i++) {
        if (fat[i] == 0) { // 空闲块
            fat[i] = EOF_BLOCK; // 标记为已分配
            return i;
        }
    }
    return 0; // 没有空闲块
}

// 释放一个块
void free_block(unsigned short block) {
    fat[block] = 0; // 标记为空闲
}

// 在当前目录中查找文件或目录
int find_file_or_dir(const char* name, DirEntry* entry) {
    DirEntry* entries = (DirEntry*)(virtual_disk + current_dir_block * BLOCK_SIZE);

    for (int i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
        if (entries[i].filename[0] != '\0' &&
            strcmp(entries[i].filename, name) == 0) {
            *entry = entries[i];
            return i;
        }
    }

    return -1; // 未找到
}

// 在打开文件表中查找空闲项
int find_empty_entry() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_file_table[i].is_used) {
            return i;
        }
    }
    return -1; // 未找到空闲项
}

// 将文件系统保存到磁盘文件
void save_to_file(const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法打开文件 %s 进行写入！\n", filename);
        return;
    }

    // 写入整个虚拟磁盘
    fwrite(virtual_disk, 1, DISK_SIZE, fp);

    fclose(fp);
    printf("文件系统已保存到 %s\n", filename);
}

// 从磁盘文件加载文件系统
void load_from_file(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("文件 %s 不存在，将创建新的文件系统！\n", filename);
        my_format();
        return;
    }

    // 释放之前的虚拟磁盘（如果存在）
    if (virtual_disk != NULL) {
        free(virtual_disk);
    }

    // 分配虚拟磁盘空间
    virtual_disk = (unsigned char*)malloc(DISK_SIZE);
    if (virtual_disk == NULL) {
        printf("内存分配失败！\n");
        fclose(fp);
        exit(1);
    }

    // 读取整个虚拟磁盘
    size_t read_size = fread(virtual_disk, 1, DISK_SIZE, fp);
    fclose(fp);

    if (read_size != DISK_SIZE) {
        printf("文件读取错误，将创建新的文件系统！\n");
        my_format();
        return;
    }

    // 重新设置全局变量
    fat = (FAT_ENTRY*)(virtual_disk + FAT_BLOCK * BLOCK_SIZE);
    current_dir_block = ROOT_BLOCK;
    strcpy(current_dir, "/");

    // 初始化打开文件表
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_file_table[i].is_used = false;
    }

    printf("文件系统已从 %s 加载！\n", filename);
}

// 主函数
int main() {
    char cmd[256];
    char arg1[256];
    char arg2[256];
    int fd, ret;
    char buffer[1024];

    // 尝试加载已有的文件系统，如果不存在则格式化一个新的
    load_from_file("filesystem.img");

    printf("简易文件系统启动成功！输入help查看可用命令。\n");

    while (1) {
        printf("%s> ", current_dir);
        fflush(stdout);

        cmd[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';

        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%s %s %s", cmd, arg1, arg2);

        if (strcmp(cmd, "format") == 0 || strcmp(cmd, "my_format") == 0) {
            my_format();
        }
        else if (strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "my_mkdir") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: mkdir <目录名>\n");
            } else {
                my_mkdir(arg1);
            }
        }
        else if (strcmp(cmd, "rmdir") == 0 || strcmp(cmd, "my_rmdir") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: rmdir <目录名>\n");
            } else {
                my_rmdir(arg1);
            }
        }
        else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "my_ls") == 0) {
            my_ls();
        }
        else if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "my_cd") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: cd <目录名>\n");
            } else {
                my_cd(arg1);
            }
        }
        else if (strcmp(cmd, "create") == 0 || strcmp(cmd, "my_create") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: create <文件名>\n");
            } else {
                my_create(arg1);
            }
        }
        else if (strcmp(cmd, "open") == 0 || strcmp(cmd, "my_open") == 0) {
            if (arg1[0] == '\0' || arg2[0] == '\0') {
                printf("用法: open <文件名> <模式(r/w/a)>\n");
            } else {
                ret = my_open(arg1, arg2[0]);
                if (ret >= 0) {
                    printf("已打开文件，文件描述符为: %d\n", ret);
                }
            }
        }
        else if (strcmp(cmd, "close") == 0 || strcmp(cmd, "my_close") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: close <文件描述符>\n");
            } else {
                fd = atoi(arg1);
                my_close(fd);
            }
        }
        else if (strcmp(cmd, "write") == 0 || strcmp(cmd, "my_write") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: write <文件描述符> [内容]\n");
                printf("      如果不提供内容，将进入多行输入模式，以单独一行的END结束\n");
            } else {
                fd = atoi(arg1);

                if (arg2[0] == '\0') {
                    // 多行输入模式
                    printf("请输入内容，以单独一行的END结束：\n");
                    char content[4096] = "";
                    char line[256];

                    while (1) {
                        fgets(line, sizeof(line), stdin);
                        if (strcmp(line, "END\n") == 0 || strcmp(line, "end\n") == 0) {
                            break;
                        }
                        strcat(content, line);
                    }

                    ret = my_write(fd, content, strlen(content));
                    if (ret >= 0) {
                        printf("已写入 %d 字节\n", ret);
                    }
                } else {
                    // 使用命令行提供的内容
                    ret = my_write(fd, arg2, strlen(arg2));
                    if (ret >= 0) {
                        printf("已写入 %d 字节\n", ret);
                    }
                }
            }
        }
        else if (strcmp(cmd, "read") == 0 || strcmp(cmd, "my_read") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: read <文件描述符> [读取字节数]\n");
            } else {
                fd = atoi(arg1);
                int size = 1024; // 默认读取1024字节

                if (arg2[0] != '\0') {
                    size = atoi(arg2);
                }

                char* read_buffer = (char*)malloc(size + 1);
                if (read_buffer == NULL) {
                    printf("内存分配失败！\n");
                    continue;
                }

                ret = my_read(fd, read_buffer, size);
                if (ret > 0) {
                    read_buffer[ret] = '\0';
                    printf("读取内容（%d 字节）：\n%s\n", ret, read_buffer);
                } else if (ret == 0) {
                    printf("已到达文件末尾\n");
                }

                free(read_buffer);
            }
        }
        else if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "my_rm") == 0) {
            if (arg1[0] == '\0') {
                printf("用法: rm <文件名>\n");
            } else {
                my_rm(arg1);
            }
        }
        else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0 ||
                 strcmp(cmd, "my_exitsys") == 0) {
            my_exitsys();
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            printf("可用命令：\n");
            printf("  format             - 格式化文件系统\n");
            printf("  mkdir <目录名>     - 创建目录\n");
            printf("  rmdir <目录名>     - 删除目录\n");
            printf("  ls                 - 显示当前目录内容\n");
            printf("  cd <目录名>        - 切换目录\n");
            printf("  create <文件名>    - 创建文件\n");
            printf("  open <文件名> <模式> - 打开文件（模式: r-读, w-写, a-追加）\n");
            printf("  close <文件描述符> - 关闭文件\n");
            printf("  write <文件描述符> [内容] - 写入文件\n");
            printf("  read <文件描述符> [字节数] - 读取文件\n");
            printf("  rm <文件名>        - 删除文件\n");
            printf("  exit/quit          - 退出文件系统\n");
        }
        else if (cmd[0] != '\0') {
            printf("未知命令: %s\n", cmd);
            printf("输入 help 查看可用命令\n");
        }
    }

    return 0;
}