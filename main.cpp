#include <map>
#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include "mytar.hpp"

int main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(stderr, "Usage: %s <src_file> <dst_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *src = argv[1], *dst = argv[2];
    struct stat srcStat, dstStat;
    if(lstat(src, &srcStat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    if(lstat(dst, &dstStat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    if(!S_ISDIR(dstStat.st_mode)){
        fprintf(stderr, "dst_pathname is not a dir\n");
        exit(EXIT_FAILURE);
    }
    int dstFd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0);

    if(S_ISDIR(srcStat.st_mode)){
        copyDir(src, dstFd);
    }else{

    }
    
    return 0;
}