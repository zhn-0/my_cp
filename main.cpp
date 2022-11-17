#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include "mytar.hpp"

int main(int argc, char *argv[])
{
    char src[256], dst[256], pwd[256];
    int op=0;
    puts("Please input 1 for Backup or 2 for Recover");
    scanf("%d", &op);
    if(op==1)
    {
        puts("Please input the file you want to backup:");
        scanf("%s", src);
        puts("Please input the position you want to save the backup file:");
        scanf("%s", dst);
        if (strcmp(src, dst) == 0)
        { 
            fprintf(stderr, "src and dst can not be same\n");
            exit(EXIT_FAILURE);
        }
        puts("Please input the password that will be used in the encrypto:");
        scanf("%s", pwd);
        generateAesKey((unsigned char*)pwd);
        backup(src, dst);
    }
    else if(op==2)
    {
        puts("Please input the file you want to recover:");
        scanf("%s", src);
        puts("Please input the decrypto password:");
        scanf("%s", pwd);
        generateAesKey((unsigned char *)pwd);
        recover(src);
    }
    else
    {
        fprintf(stderr, "Wrong operation\n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}