#include "mytar.hpp"

static std::map<ino_t, std::string> inodeToPath;

// 填补空白
static void writePadding(int dstFd, size_t size)
{
    int padd = size % BLOCKSIZE;
    unsigned char c = 0;
    while(padd < BLOCKSIZE)
    {
        write(dstFd, &c, sizeof(c));
        ++padd;
    }
}

// 计算TarHeader的校验和
static void calHeaderChecksum(union TarHeader *pheader)
{
    unsigned char chksum = 0;
    for (int i = 0; i < BLOCKSIZE; ++i)
        chksum += (unsigned char)pheader->block[i];
    sprintf(pheader->checksum, "%o", chksum);
}

static void calFileChecksum(union TarHeader *pheader, int fd, off_t fileBeg)
{
    unsigned char chksum = 0;
    char c;
    lseek(fd, fileBeg, SEEK_SET);
    while(read(fd, &c, sizeof(c)) > 0)
    {
        chksum += (unsigned char)c;
    }
    sprintf(pheader->devMajor, "%o", chksum);
}

static void fillTarHeader(union TarHeader *pheader, char *src)
{
    struct stat srcStat;
    lstat(src, &srcStat);
    struct passwd *pwd;
    struct group *grp;
    pwd = getpwuid(srcStat.st_uid);
    grp = getgrgid(srcStat.st_gid);
    char tmpPrefix[155], tmpBuf[155];
    memset(pheader->block, 0, sizeof(pheader->block));

    sprintf(pheader->name, "%s", basename(src));
    sprintf(pheader->mode, "%o", srcStat.st_mode);
    sprintf(pheader->uid, "%o", srcStat.st_uid);
    sprintf(pheader->gid, "%o", srcStat.st_gid);
    sprintf(pheader->size, "%lo", srcStat.st_size);
    sprintf(pheader->mtime, "%lo", srcStat.st_mtim.tv_sec);
    sprintf(pheader->ustar, "ustar");
    sprintf(pheader->owner, "%s", pwd->pw_name);
    sprintf(pheader->group, "%s", grp->gr_name);
    
    strcpy(tmpPrefix, src);
    sprintf(pheader->prefix, "%s", realpath(dirname(tmpPrefix), tmpBuf));

    if(S_ISDIR(srcStat.st_mode))
    {
        sprintf(&pheader->type, "%c", DIRECTORY);
        sprintf(pheader->size, "%lo", srcStat.st_size);
    }
    else if(S_ISREG(srcStat.st_mode))
    {
        sprintf(&pheader->type, "%c", NORMAL);
    }
    else if(S_ISLNK(srcStat.st_mode))
    {
        sprintf(&pheader->type, "%c", SYMLINK);
    }
    else if(S_ISFIFO(srcStat.st_mode))
    {
        sprintf(&pheader->type, "%c", FIFO);
    }
}

int copyLink(char *src, int dstFd, ino_t inode)
{
    union TarHeader header;
    fillTarHeader(&header, src);
    sprintf(&header.type, "%c", HARDLINK);
    sprintf(header.size, "%lo", 0UL);
    sprintf(header.linkName, "%s", inodeToPath[inode].c_str());

    calHeaderChecksum(&header);

    write(dstFd, header.block, sizeof(header.block));
    return 0;
}

int copySymlink(char *src, int dstFd)
{
    union TarHeader header;
    fillTarHeader(&header, src);

    sprintf(header.size, "%lo", 0UL);
    readlink(src, header.linkName, 100);

    calHeaderChecksum(&header);

    write(dstFd, header.block, sizeof(header.block));
    return 0;
}

int copyFifo(char *src, int dstFd)
{
    union TarHeader header;
    fillTarHeader(&header, src);

    sprintf(header.size, "%lo", 0UL);

    calHeaderChecksum(&header);

    write(dstFd, header.block, sizeof(header.block));
    return 0;
}

int copyReg(char *src, int dstFd)
{
    int srcFd;
    struct stat srcStat;
    if((srcFd = open(src, O_RDONLY)) < 0)
    {
        fprintf(stderr, "%s: open failed\n\n", src);
        exit(EXIT_FAILURE);
    }
    stat(src, &srcStat);

    // 辨别硬链接
    if(inodeToPath.count(srcStat.st_ino))
    {
        return copyLink(src, dstFd, srcStat.st_ino);
    }

    union TarHeader header;

    off_t headerPos, fileBeg, fileEnd;
    headerPos = lseek(dstFd, 0, SEEK_CUR);
    // 预写header
    write(dstFd, header.block, sizeof(header.block));

    fileBeg = lseek(dstFd, 0, SEEK_CUR);
    if(srcStat.st_size)compress(srcFd, dstFd);
    fileEnd = lseek(dstFd, 0, SEEK_CUR);

    // 获得压缩文件大小
    size_t size = fileEnd - fileBeg;
    writePadding(dstFd, size);

    fillTarHeader(&header, src);
    calFileChecksum(&header, dstFd, fileBeg);
    sprintf(header.size, "%lo", size);
    calHeaderChecksum(&header);

    inodeToPath[srcStat.st_ino] = std::string(header.prefix) + "/" + std::string(header.name);

    lseek(dstFd, headerPos, SEEK_SET);
    write(dstFd, header.block, sizeof(header.block));
    lseek(dstFd, 0, SEEK_END);

    close(srcFd);
    return 0;
}

int copyDir(char *src, int dstFd)
{
    DIR *srcDir;
    if((srcDir = opendir(src)) == NULL){
        fprintf(stderr, "%s: open failed\n", src);
        exit(EXIT_FAILURE);
    }

    struct stat srcStat;
    stat(src, &srcStat);

    union TarHeader header;
    fillTarHeader(&header, src);
    calHeaderChecksum(&header);
    write(dstFd, header.block, sizeof(header.block));
    struct dirent *copyFile;
    struct stat copyFileStat;
    char copyFileSrc[512];
    while((copyFile = readdir(srcDir)) != NULL){
        if(strcmp(copyFile->d_name, ".") == 0 || strcmp(copyFile->d_name, "..") == 0)continue;
        sprintf(copyFileSrc, "%s/%s", src, copyFile->d_name);
        lstat(copyFileSrc, &copyFileStat);
        if(S_ISDIR(copyFileStat.st_mode))
        {
            copyDir(copyFileSrc, dstFd);
        }
        else if(S_ISREG(copyFileStat.st_mode))
        {
            copyReg(copyFileSrc, dstFd);
        }
        else if(S_ISLNK(copyFileStat.st_mode))
        {
            copySymlink(copyFileSrc, dstFd);
        }
        else if(S_ISFIFO(copyFileStat.st_mode))
        {
            copyFifo(copyFileSrc, dstFd);
        }
    }

    closedir(srcDir);
    return 0;
}

int backup(char *src, char *dst)
{
    struct stat srcStat;
    if(lstat(src, &srcStat)!=0)
    {
        printf("%s not exist\n", src);
        exit(EXIT_FAILURE);
    }
    int tmpFd = open(".tmpfile_for_encrypto", O_RDWR | O_CREAT | O_TRUNC, 0777);
    int dstFd = open(dst, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (S_ISDIR(srcStat.st_mode))
    {
        copyDir(src, tmpFd);
    }
    else if (S_ISREG(srcStat.st_mode))
    {
        copyReg(src, tmpFd);
    }
    else if (S_ISLNK(srcStat.st_mode))
    {
        copySymlink(src, tmpFd);
    }
    else if (S_ISFIFO(srcStat.st_mode))
    {
        copyFifo(src, tmpFd);
    }
    
    lseek(tmpFd, 0, SEEK_SET);
    encrypt(tmpFd, dstFd);
    
    close(tmpFd);
    unlink(".tmpfile_for_encrypto");
    return 0;
}

class DirTime
{
public:
    char path[256];
    time_t time;
    struct DirTime *nxt;
    DirTime(char *src, time_t t)
    {
        if(src)strcpy(path, src);
        time = t;
        nxt = NULL;
    }
    ~DirTime()
    {
        if(nxt)delete nxt;
    }
};

// 用校验和检验文件是否出错
void checkHeaderChecksum(union TarHeader *pheader)
{
    unsigned char chksum = 0, rchksum = 0;
    sscanf(pheader->checksum, "%hho", &rchksum);
    for(int i=0;i<BLOCKSIZE;++i)
    {
        if(i>=148 && i<156)continue;
        chksum += (unsigned char)pheader->block[i];
    }
    if(chksum != rchksum)
    {
        fprintf(stderr, "%s: corruption\n", pheader->name);
        exit(EXIT_FAILURE);
    }
}

void checkFileChecksum(union TarHeader *pheader, int fd, size_t sz)
{
    off_t fileBeg = lseek(fd, 0, SEEK_CUR);
    unsigned char chksum = 0, rchksum = 0;
    char c;
    sscanf(pheader->devMajor, "%hho", &rchksum);
    while(sz--)
    {
        read(fd, &c, sizeof(c));
        chksum += (unsigned char)c;
    }
    if(chksum != rchksum)
    {
        fprintf(stderr, "%s: corruption\n", pheader->name);
        exit(EXIT_FAILURE);
    }
    lseek(fd, fileBeg, SEEK_SET);
}

int unpackFile(int srcFd)
{
    uid_t uid;
    gid_t gid;
    DIR *dstDir;
    mode_t mode;
    size_t size;
    struct timespec times[2];
    int dstFd;
    char type, dst[256];
    times[0].tv_nsec = UTIME_NOW;
    
    union TarHeader header;
    struct DirTime *vhead = new DirTime(NULL, 0), *p = vhead;
    while(read(srcFd, header.block, sizeof(header.block)) == 512)
    {
        checkHeaderChecksum(&header);
        type = header.type;
        sscanf(header.uid, "%o", &uid);
        sscanf(header.gid, "%o", &gid);
        sscanf(header.mode, "%o", &mode);
        sscanf(header.size, "%lo", &size);
        sscanf(header.mtime, "%lo", &times[1].tv_sec);
        sprintf(dst, "%s/%s", header.prefix, header.name);
        if(type == NORMAL)
        {
            unlink(dst);
            if((dstFd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0)
            {
                fprintf(stderr, "%s: create failed\n", dst);
                exit(EXIT_FAILURE);
            }
            if(fchown(dstFd, uid, gid) < 0)
            {
                fprintf(stderr, "%s: fchown failed", dst);
                exit(EXIT_FAILURE);
            }
            checkFileChecksum(&header, srcFd, size);
            if(size)uncompress(srcFd, dstFd, size);
            // 不足512字节会补零 需跳过
            off_t oft = (512 - size % 512) % 512;
            lseek(srcFd, oft, SEEK_CUR);
            futimens(dstFd, times);
            close(dstFd);
        }
        else if(type == DIRECTORY)
        {
            if ((dstDir = opendir(dst)) == NULL)
            {
                if(mkdir(dst, mode) < 0)
                {
                    fprintf(stderr, "%s: mkdir failed\n", dst);
                    exit(EXIT_FAILURE);
                }
                if (chown(dst, uid, gid) < 0)
                {
                    fprintf(stderr, "%s: chown failed\n", dst);
                    exit(EXIT_FAILURE);
                }
                dstDir = opendir(dst);
            }
            p->nxt = new DirTime(dst, times[1].tv_sec);
            p = p->nxt;
            closedir(dstDir);
        }
        else if(type == HARDLINK)
        {
            link(header.linkName, dst);
        }
        else if(type == SYMLINK)
        {
            symlink(header.linkName, dst);
            utimensat(AT_FDCWD, dst, times, AT_SYMLINK_NOFOLLOW);
        }
        else if(type == FIFO)
        {
            mkfifo(dst, mode);
            utimensat(AT_FDCWD, dst, times, AT_SYMLINK_NOFOLLOW);
        }
    }

    p = vhead->nxt;
    while(p)
    {
        times[1].tv_sec = p->time;
        utimensat(AT_FDCWD, p->path, times, AT_SYMLINK_NOFOLLOW);
        p = p->nxt;
    }
    delete vhead;

    return 0;
}

int recover(char *src)
{
    int tmpFd = open(".tmpfile_for_decrypto", O_RDWR | O_CREAT | O_TRUNC, 0777);
    int srcFd = open(src, O_RDONLY);

    decrypt(srcFd, tmpFd);

    lseek(tmpFd, 0, SEEK_SET);
    unpackFile(tmpFd);

    unlink(".tmpfile_for_decrypto");

    close(tmpFd);
    close(srcFd);

    return 0;
}