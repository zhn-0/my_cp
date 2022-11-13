#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/time.h>
#include "mytar.hpp"

int main(int argc, char *argv[])
{
    generateAesKey((unsigned char *)"hello");
    if(argv[1][0] == '0')
    {
        int dstFd = open("tmp", O_RDWR | O_CREAT | O_TRUNC, 0777);
        backup("../compiler", dstFd);
        // backup("tmpf", dstFd);
    }
    else recover("tmp");

    return 0;
}