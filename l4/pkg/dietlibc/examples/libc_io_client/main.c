#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/dietlibc/attachable.h>
#include <l4/dietlibc/name_server.h>

#include <dice/dice.h> // for CORBA_*

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 1024

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt)
{
    free(prt);
}

int main(int argc, char* argv[])
{
    struct dirent * dir;
    DIR * D;
    l4_threadid_t ns;
    int fd, ret;
    char buf[BUF_SIZE];

    l4_sleep(1000);
    LOG_init("libc_iot");

    D = opendir("/");
    if (D == NULL)
    {
        LOG("errno = %d", errno);
        perror("opendir");
        LOG_flush();
        return 1;
    }
//    LOG("D = %p", D);
    while ((dir = readdir(D)))
    {
        LOG("%s (%d)", dir->d_name, dir->d_ino);
        LOG_flush();
    }
    closedir(D);

    D = opendir("/test3/");
    if (D == NULL)
    {
        LOG("errno = %d", errno);
        perror("opendir");
        LOG_flush();
        return 1;
    }
//    LOG("D = %p", D);
    while ((dir = readdir(D)))
    {
        LOG("%s (%d)", dir->d_name, dir->d_ino);
        LOG_flush();
    }
    closedir(D);

    // mount server now
    ns = get_name_server_threadid();
    ret = attach_namespace(ns, 56, "/", "/linux");
    LOG("attach_namespace() = %d", ret);

    fd = open("/linux/hosts", O_RDONLY);
    LOG("open() = %d", fd);
    if (errno != 0)
        LOG("errno = %d", errno);

    ret = read(fd, &buf, BUF_SIZE);
    LOG("read() = %d", ret);

    if (ret >= 0)
    {
        buf[ret] = 0;
        LOG("read.buf = '%s'", buf);
    }
    return 0;
}
