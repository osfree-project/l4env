/**
 * \file   l4vfs/simple_file_server/examples/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/uio.h>

#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/common_io.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>

#include <l4/simple_file_server/simple_file_server.h>

char *test_text = "LAZY_TEST_TEXT_LAZY_TEST_TEXT";

#define BUF_SIZE 1024
#define COUNT 64

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
    l4_threadid_t ns, vs;
    int fd, fd1, ret;
    void *mem_buf;
    object_id_t res,base;
    char *mmap_buf;
    unsigned char buf[BUF_SIZE];
    char buf0[20];
    char buf1[100];
    char buf2[300];
    int iovcnt;
    struct iovec iov[3];

    fd_set rfds;
    struct timeval tv;

    iov[0].iov_base = buf0;
    iov[0].iov_len = sizeof(buf0);
    iov[1].iov_base = buf1;
    iov[1].iov_len = sizeof(buf1);
    iov[2].iov_base = buf2;
    iov[2].iov_len = sizeof(buf2);

    iovcnt = sizeof(iov) / sizeof(struct iovec);

    /********** VOLUME TEST ************************************/

    openlog("SFS_TEST", LOG_PID, 0);
    syslog(LOG_DEBUG,"Volume stuff: *******************");

    ns = l4vfs_get_name_server_threadid();
    l4_sleep(1500);

    base.volume_id=L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id=0;

    vs = l4vfs_thread_for_volume(ns,SIMPLE_FILE_SERVER_VOLUME_ID);
    syslog(LOG_DEBUG,"Volume server for volume %d (0.0) '" l4util_idfmt "'",
           SIMPLE_FILE_SERVER_VOLUME_ID,l4util_idstr(vs));

    res.volume_id=SIMPLE_FILE_SERVER_VOLUME_ID;
    res.object_id=1;

    syslog(LOG_DEBUG,"try to open file with object_id %d using basic_io_open",
           res.object_id);

    ret = l4vfs_attach_namespace(ns, SIMPLE_FILE_SERVER_VOLUME_ID, "/",
                                 "/server");

    syslog(LOG_DEBUG,"attach_namespace() = %d", ret);

    /*********** BASIC_IO TEST ************************************/

    fd = l4vfs_open(vs,res,0);
    syslog(LOG_DEBUG,"BASIC_IO stuff: *******************");

    syslog(LOG_DEBUG,"file handle after open file should be >= 0, fd = %d",fd);

    if (fd >= 0)
    {
        syslog(LOG_DEBUG,"try to close file using bacic_io_close");
        ret = l4vfs_close(vs,fd);
        syslog(LOG_DEBUG,"ret after close file: %d",ret);
    }

    /**************** SELECT TEST ******************************/

    syslog(LOG_DEBUG,"Select stuff: *******************");

    syslog(LOG_DEBUG,"try to open hosts file");
    fd = open("/server/hosts", O_RDONLY);
    syslog(LOG_DEBUG,"fd should be >= 0, fd = %d", fd);

    syslog(LOG_DEBUG,"try to open povray.ini file");
    fd1 = open("server/povray.ini", O_RDONLY);
    syslog(LOG_DEBUG,"fd should be >= 0, fd = %d", fd1);

    /* Watch fd to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    FD_SET(fd1, &rfds);

    /* Wait up to ten seconds. */
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    ret = select(2, &rfds, NULL, NULL, &tv);

    syslog(LOG_DEBUG,"ret after select should be 2 and is %d",ret);
    syslog(LOG_DEBUG,"fd_isset(%d) should be 1 and is %d",fd,
           FD_ISSET(fd, &rfds));
    syslog(LOG_DEBUG,"fd_isset(%d) should be 1 and is %d",fd1,
           FD_ISSET(fd1, &rfds));

    FD_ZERO(&rfds);
    FD_SET(fd1, &rfds);

    ret = select(2, &rfds, NULL, NULL, &tv);

    syslog(LOG_DEBUG,"ret after second select should be 1 and is %d",ret);


    /******************* READ/READV TEST *******************************/

    syslog(LOG_DEBUG,"Read/Readv stuff: *******************");

    syslog(LOG_DEBUG,"try to read from hosts");
    while ((ret = read(fd, buf, COUNT)))
    {
        buf[ret] = '\0';
        syslog(LOG_DEBUG,"%s",buf);
        l4_sleep(10);
    }

    lseek(fd,0,SEEK_SET);

    syslog(LOG_DEBUG,"try to read from hosts by readv, result should be != -1");
    ret = readv(fd,iov, iovcnt);
    syslog(LOG_DEBUG,"result is: %d",ret);

    mem_buf = NULL;

    /******************** MMAP/MSYNC TEST **********************************/

    syslog(LOG_DEBUG,"MMAP stuff: *******************");

    syslog(LOG_DEBUG,"try to make a mmap with wrong fd, should return NULL");
    mem_buf = mmap(NULL,COUNT,PROT_WRITE,0,-4,0);

    mem_buf = NULL;

    syslog(LOG_DEBUG,"%s %s","try to make mmap with wrong start address and",
           "MAP_FIXED, should return NULL");

    mem_buf = mmap((void *)1024,COUNT,PROT_WRITE,MAP_FIXED,fd,0);

    mem_buf = NULL;

    syslog(LOG_DEBUG,"try to make a MAP_PRIVATE mmap, should return mem address");

    mem_buf = mmap(NULL,128,PROT_WRITE,MAP_PRIVATE,fd,32);

    if (mem_buf != NULL) 
    {
        mmap_buf = (char *) mem_buf;
        syslog(LOG_DEBUG,"successfully MAP_PRIVATE mmap, content of mmap_buf: ");
        syslog(LOG_DEBUG,"%s",mmap_buf);
    }

    mem_buf = NULL;

    syslog(LOG_DEBUG,"%s %s","try to make a MAP_SHARED mmap, should return",
           "memory address and print content");
    mem_buf = mmap(NULL,64,PROT_WRITE,MAP_SHARED,fd,0);

    if (mem_buf != NULL)
    {
        mmap_buf = (char *) mem_buf;
        mmap_buf[63] = '\0';
        syslog(LOG_DEBUG,"successfully MAP_SHARED mmap, content of buf: ");
        syslog(LOG_DEBUG,"%s",mmap_buf);

        syslog(LOG_DEBUG,"%s %s","now try to write something in mmap memory",
               "buffer, should work without page fault");

        strncpy(mmap_buf,test_text,29);

        syslog(LOG_DEBUG,"try to make msync");
        ret = msync(mmap_buf,64,0);

        syslog(LOG_DEBUG,"ret after msync should be 0 and is %d",ret);
   }

    syslog(LOG_DEBUG,"try to close file handle");
    ret = close(fd);
    syslog(LOG_DEBUG,"ret after close should be 0 and is %d",ret);

    syslog(LOG_DEBUG,"%s %s","try to open and read from hosts which was",
           "changed after mmap and msync");

    fd = open("/server/hosts", O_RDONLY);
    syslog(LOG_DEBUG,"open() = %d", fd);

    ret = read(fd, buf, COUNT);

    syslog(LOG_DEBUG,"ret after read should be: %d, and is %d",COUNT,ret);

    if (ret > 0)
    {
        buf[ret] = 0;
        syslog(LOG_DEBUG,"%s",buf);
    }

    mem_buf = NULL;

    /**************** ANONYM MMAP TEST *****************************************/

    syslog(LOG_DEBUG,"Anonym mmap stuff: *******************");

    syslog(LOG_DEBUG,"try to get anonym mem via mmap, should return start address");
    mem_buf = mmap(0,1024,PROT_WRITE, MAP_ANON, 0, 0);
    if (mem_buf != NULL)
    {
        syslog(LOG_DEBUG,"got anonym mem successfully");
    }

    syslog(LOG_DEBUG,"%s %s","now try to write something in anonymous mmap",
          "memory buffer should work without page fault\n");
    strncpy(mem_buf,test_text,29);

    syslog(LOG_DEBUG,"end of simple_file_server test");

    return 0;
}

