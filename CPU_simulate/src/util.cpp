#include "util.h"
using namespace std;

int Open(const char* file, int flag, int perm)
{
    int fd;
    if ((fd=open(file,flag,perm))<0)
    {
        LOG_ERROR("Open file error!\n");
        LOG_ERROR("Can not open file: %s\n", file);
        exit(-1);
    }
    return fd;
}

void Write(int fd, const void* buf, size_t len)
{
    if (write(fd,buf,len)<0)
    {
        LOG_ERROR("Write error!\n");
        exit(-1);
    }
}