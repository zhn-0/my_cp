#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include "mytar.hpp"

int main(int argc, char *argv[])
{
    printf("\033c");
    char src[256], dst[256], pwd[256];
    int op=0;
    puts("Please input 1 for Backup or 2 for Recover");
    scanf("%d", &op);
    printf("\033c");
    if(op==1)
    {
        while(1)
        {
            puts("Please input the file you want to backup:");
            scanf("%s", src);
            struct stat st;
            printf("\033c");
            if(lstat(src, &st)==0)break;
            printf("%s not exist, please retry\n", src);
        }
        puts("Please input the position you want to save the backup file:");
        scanf("%s", dst);
        printf("\033c");
        if (strcmp(src, dst) == 0)
        { 
            fprintf(stderr, "src and dst can not be same\n");
            exit(EXIT_FAILURE);
        }
        puts("Please input the password that will be used in the encrypto:");
        scanf("%s", pwd);
        printf("\033c");
        generateAesKey((unsigned char*)pwd);
        backup(src, dst);
        puts("backup over");
    }
    else if(op==2)
    {
        while(1)
        {
            puts("Please input the file you want to recover:");
            scanf("%s", src);
            struct stat st;
            printf("\033c");
            if(lstat(src, &st)==0)break;
            printf("%s not exist, please retry\n", src);
        }
        puts("Please input the decrypto password:");
        scanf("%s", pwd);
        printf("\033c");
        generateAesKey((unsigned char *)pwd);
        recover(src);
        puts("recover over");
    }
    else
    {
        fprintf(stderr, "Wrong operation\n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}