#include "mytar.hpp"

static std::map<ino_t, std::string> inodeToPath;

// 填补空白
void writePadding(int dstFd, size_t size)
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
void calChecksum(union TarHeader *pheader)
{
    unsigned char chksum = 0;
    for (int i = 0; i < BLOCKSIZE; ++i)
        chksum += (unsigned char)pheader->block[i];
    sprintf(pheader->checksum, "%o", chksum);
}

void fillTarHeader(union TarHeader *pheader, char *src)
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
}

int copyLink(char *src, int dstFd, ino_t inode)
{
    union TarHeader header;
    fillTarHeader(&header, src);
    sprintf(&header.type, "%c", HARDLINK);
    sprintf(header.size, "%lo", 0UL);
    sprintf(header.linkName, "%s", inodeToPath[inode].c_str());

    calChecksum(&header);

    write(dstFd, header.block, sizeof(header.block));
    return 0;
}

int copySymlink(char *src, int dstFd)
{
    union TarHeader header;
    fillTarHeader(&header, src);

    sprintf(header.size, "%lo", 0UL);
    readlink(src, header.linkName, 100);

    calChecksum(&header);

    write(dstFd, header.block, sizeof(header.block));
    return 0;
}

int copyReg(char *src, int dstFd)
{
    int srcFd;
    struct stat srcStat;
    if((srcFd = open(src, O_RDONLY)) < 0)
    {
        fprintf(stderr, "%s failed to open\n", src);
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
    size_t ret = compress(srcFd, dstFd);
    fileEnd = lseek(dstFd, 0, SEEK_CUR);

    // 获得压缩文件大小
    size_t size = fileEnd - fileBeg;
    writePadding(dstFd, size);

    fillTarHeader(&header, src);
    sprintf(header.size, "%lo", size);
    calChecksum(&header);

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
        fprintf(stderr, "%s failed to open\n", src);
        exit(EXIT_FAILURE);
    }

    struct stat srcStat;
    stat(src, &srcStat);

    union TarHeader header;
    fillTarHeader(&header, src);
    calChecksum(&header);
    write(dstFd, header.block, sizeof(header.block));
    struct dirent *copyFile;
    struct stat copyFileStat;
    char copyFileSrc[512];
    while((copyFile = readdir(srcDir)) != NULL){
        if(strcmp(copyFile->d_name, ".") == 0 || strcmp(copyFile->d_name, "..") == 0)continue;
        sprintf(copyFileSrc, "%s/%s", src, copyFile->d_name);
        lstat(copyFileSrc, &copyFileStat);
        if(S_ISDIR(copyFileStat.st_mode)){
            copyDir(copyFileSrc, dstFd);
        }else if(S_ISREG(copyFileStat.st_mode)){
            copyReg(copyFileSrc, dstFd);
        }else if(S_ISLNK(copyFileStat.st_mode)){
            copySymlink(copyFileSrc, dstFd);
        }
    }

    closedir(srcDir);
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

int unpackFile(char *src)
{
    uid_t uid;
    gid_t gid;
    DIR *dstDir;
    mode_t mode;
    size_t size;
    struct timespec times[2];
    int srcFd, dstFd;
    char type, dst[256];
    srcFd = open(src, O_RDONLY);
    times[0].tv_nsec = UTIME_NOW;
    
    union TarHeader header;
    struct DirTime *vhead = new DirTime(NULL, 0), *p = vhead;
    while(read(srcFd, header.block, sizeof(header.block)) == 512)
    {
        type = header.type;
        sscanf(header.uid, "%o", &uid);
        sscanf(header.gid, "%o", &gid);
        sscanf(header.mode, "%o", &mode);
        sscanf(header.size, "%lo", &size);
        sscanf(header.mtime, "%lo", &times[1].tv_sec);
        sprintf(dst, "%s/%s", header.prefix, header.name);
        if(type == NORMAL)
        {
            if((dstFd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0)
            {
                perror("creat");
                return -1;
            }
            if(fchown(dstFd, uid, gid) < 0)
            {
                perror("fchown");
                return -1;
            }
            uncompress(srcFd, dstFd, size);
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
                    return -1;
                }
                if (chown(dst, uid, gid) < 0)
                {
                    fprintf(stderr, "%s: chown failed\n", dst);
                    return -1;
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
    }
    close(srcFd);

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