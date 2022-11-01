#ifndef MYTAR
#define MYTAR

#include <map>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/time.h>
#include "crypto.hpp"
#include "Huffman.hpp"

#define BLOCKSIZE   512

// 文件类型
#define REGULAR      0
#define NORMAL      '0'
#define HARDLINK    '1'
#define SYMLINK     '2'
#define CHAR        '3'     // 字符设备文件
#define BLOCK       '4'     // 块设备文件
#define DIRECTORY   '5'
#define FIFO        '6'     // 管道文件

union TarHeader
{
    union
    {
        struct
        {
            char name[100];     // 文件名
            char mode[8];       // mode_t 8进制
            char uid[8];        // user id 8进制
            char gid[8];        // group id 8进制
            char size[12];      // 文件大小 8进制
            char mtime[12];     // 上次修改时间 8进制
            char checksum[8];   // 校验和
            char link;          // 是否为链接文件
            char linkName[100]; // 链接的文件名
        };

        struct
        {
            char old[156];          // 上形式的前156字节
            char type;              // 文件类型
            char alsoLinkName[100]; // 链接的文件名
            char ustar[8];          // 魔数 若为该header则应为ustar\000
            char owner[32];         // user name
            char group[32];         // group name
            char devMajor[8];       // 主设备号
            char devMinor[8];       // 副设备号
            char prefix[155];       // 文件路径
            char padding[12];       // 填充字节
        };
    };

    char block[512];
};

int backup(char *src, int dstFd);
int recover(char *src);

#endif