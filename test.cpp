#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include "mytar.hpp"

void pack()
{
    int dstFd = open("tmpFile", O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(dstFd == -1)
    {
        perror("open");
    }
    copyDir((char*)"/home/zhn/cp/asd", dstFd);
    close(dstFd);
}

int main(int argc, char *argv[])
{
    if(argv[1][0] == '0')pack();
    else if(argv[1][0] == '1')unpackFile("tmpFile");

    return 0;
}