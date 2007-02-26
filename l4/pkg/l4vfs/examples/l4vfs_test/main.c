/**
 * \file   l4vfs/examples/l4vfs_test/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/volume_ids.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>

#include <dice/dice.h> // for CORBA_*

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

char LOG_tag[9]="l4vfs_te";

#define BUF_SIZE 1024

#define MIN(a, b) ((a)<(b)?(a):(b))

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
    int fd, ret, i;
    char buf[BUF_SIZE];
    char * cwd;

    struct timeval tv;

    l4_sleep(1000);
    cwd = getcwd(buf, BUF_SIZE);
    LOG("cwd = '%s', errno = %d", cwd, errno);

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
        LOG("%s (%ld)", dir->d_name, dir->d_ino);
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
        LOG("%s (%ld)", dir->d_name, dir->d_ino);
        LOG_flush();
    }
    closedir(D);

    // mount server now
    ns = l4vfs_get_name_server_threadid();
    ret = l4vfs_attach_namespace(ns, STATIC_FILE_SERVER_VOLUME_ID,
                                 "/", "/linux");
    LOG("attach_namespace() = %d", ret);

    // open and read a bit ...
    fd = open("/linux/hosts", O_RDONLY);
    LOG("open() = %d", fd);
    if (errno != 0)
        LOG("errno = %d", errno);

    ret = read(fd, buf, BUF_SIZE);
    LOG("read() = %d", ret);
    if (ret >= 0)
    {
        buf[MIN(ret, BUF_SIZE)] = 0;
        LOG("read.buf = '%s'", buf);
    }
    ret = close(fd);
    LOG("close() = %d", ret);

    // open and read a bit ...
    // this time via another point
    fd = open("/file1", O_RDONLY);
    LOG("open() = %d", fd);
    if (errno != 0)
        LOG("errno = %d", errno);

    ret = read(fd, buf, BUF_SIZE);
    LOG("read() = %d", ret);
    if (ret >= 0)
    {
        buf[MIN(ret, BUF_SIZE)] = 0;
        LOG("read.buf = '%s'", buf);
    }
    ret = close(fd);
    LOG("close() = %d", ret);

    // chdir checks
    ret = chdir("/");
    LOG("1. chdir(\"/\"): ret = %d, errno = %d", ret, errno);
    cwd = getcwd(buf, BUF_SIZE);
    LOG("1. getcwd(): cwd = '%s', errno = %d", cwd, errno);

    ret = chdir("/linux");
    LOG("2. chdir(\"/linux\"): ret = %d, errno = %d", ret, errno);
    cwd = getcwd(buf, BUF_SIZE);
    LOG("2. getcwd(): cwd = '%s', errno = %d", cwd, errno);

    ret = chdir("../server");
    LOG("3. chdir(\"../server\"): ret = %d, errno = %d", ret, errno);
    cwd = getcwd(buf, BUF_SIZE);
    LOG("3. getcwd(): cwd = '%s', errno = %d", cwd, errno);

    // time checks
    for (i = 0; i < 5; i++)
    {
        ret = gettimeofday(&tv, NULL);
        if (ret != 0)
        {
            LOG("gettimeofday() failed");
        }
        LOG("Time: %ld,%ld", tv.tv_sec, tv.tv_usec / 1000);
        l4_sleep((i + 1) * 100);
    }

    return 0;
}
