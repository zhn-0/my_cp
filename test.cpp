#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include "mytar.hpp"

int main(int argc, char *argv[])
{
    // char path[] = "tmpFile";
    // if(argv[1][0] == '0')pack();
    // else if(argv[1][0] == '1')recover(path);
    // test();

    generateAesKey((unsigned char *)"hello");
    if(argv[1][0] == '0')
    {
        int dstFd = open("tmp", O_RDWR | O_CREAT | O_TRUNC, 0777);
        backup("./asd", dstFd);
    }
    else recover("tmp");

    return 0;
}