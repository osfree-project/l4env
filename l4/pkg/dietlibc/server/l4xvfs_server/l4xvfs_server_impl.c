#include <l4/dietlibc/l4xvfs_lib.h>

#include <l4xvfs-server.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/util/l4_macros.h>
//#include <l4/log/l4log.h>

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

#ifdef DEBUG
#define DEBUG_printf(...) printf(__VA_ARGS__)
#else
#define DEBUG_printf(...)
#endif

#define READ_BUF_SIZE 4096
char read_buf[READ_BUF_SIZE];

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt)
{
    free(prt);
}

l4_int32_t l4x_vfs_open_component(CORBA_Object _dice_corba_obj,
                                  const char* name,
                                  l4_int32_t flags,
                                  l4_int32_t *errno_wrap,
                                  CORBA_Environment *_dice_corba_env)
{
    int temp;
    
    DEBUG_printf("Trying to open '%s'.\n", name);
    temp = open(name, flags);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in open:");
    }
    DEBUG_printf("Got fd '%d'.\n", temp);

    return temp;
}


ssize_t l4x_vfs_read_component(CORBA_Object _dice_corba_obj,
                               l4_int32_t fd,
                               l4_int8_t **buf,
                               size_t *count,
                               l4_int32_t *errno_wrap,
                               CORBA_Environment *_dice_corba_env)
{
    int temp;
    *buf = read_buf;
    *count = MIN(*count, READ_BUF_SIZE);
    DEBUG_printf("Trying to read %d bytes from fd '%d' into buffer at %p\n",
           *count, fd, *buf);

    if (errno)
        perror("Error before read:");
    temp = read(fd, *buf, *count);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error after read:");
    }
    DEBUG_printf("Got '%d' bytes.\n", temp);
    if (temp > 0)
        *count = temp;

    return temp;
}


ssize_t l4x_vfs_write_component(CORBA_Object _dice_corba_obj,
                                l4_int32_t fd,
                                const l4_int8_t *buf,
                                size_t *count,
                                l4_int32_t *errno_wrap,
                                CORBA_Environment *_dice_corba_env)
{
    int temp;
    temp = write(fd, buf, *count);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in write:");
    }

    //CORBA_free(buf);
    return temp;
}


l4_int32_t l4x_vfs_close_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t fd,
                                   l4_int32_t *errno_wrap,
                                   CORBA_Environment *_dice_corba_env)
{
    int temp;
    temp = close(fd);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in close:");
    }

    return temp;
}


l4_int32_t l4x_vfs_fsync_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t fd,
                                   l4_int32_t *errno_wrap,
                                   CORBA_Environment *_dice_corba_env)
{
    int temp;
    temp = fsync(fd);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in fsync:");
    }

    return temp;
}


off_t l4x_vfs_lseek_component(CORBA_Object _dice_corba_obj,
                              l4_int32_t fd,
                              off_t offset,
                              l4_int32_t whence,
                              l4_int32_t *errno_wrap,
                              CORBA_Environment *_dice_corba_env)
{
    int temp;
    DEBUG_printf("Before seek()\n");
    temp = lseek(fd, offset, whence);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in lseek:");
    }
    DEBUG_printf("After seek()\n");

    return temp;
}


l4_int32_t l4x_vfs_fstat_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t fd,
                                   struct stat *buf,
                                   l4_int32_t *errno_wrap,
                                   CORBA_Environment *_dice_corba_env)
{
    int temp;
    temp = fstat(fd, buf);
    if (errno)
    {
        *errno_wrap = errno;
        perror("Error in fstat:");
    }

    return temp;
}


void l4x_vfs_close_connection_component(CORBA_Object _dice_corba_obj,
                                        CORBA_Environment *_dice_corba_env)
{
    DEBUG_printf("Closing connection and exiting worker thread ...\n");
    exit(0);
}


l4_threadid_t
l4x_connection_init_connection_component(CORBA_Object _dice_corba_obj,
                                         CORBA_Environment *_dice_corba_env)
{
    pid_t pid;
    int pipe_fd[2];
    int ret;
    l4_threadid_t me;

    // fork here:
    // parent should return childs l4thread_id
    // child should enter the core server loop

    DEBUG_printf("Initing connection, creating pipe ...\n");

    ret = pipe(pipe_fd);
    if (ret != 0)
    {
        perror("Error creating pipe :");
        return L4_INVALID_ID;
    }
    
    DEBUG_printf("Forking ...\n");
    pid = fork();
    if (pid == 0) // child
    {
        // send l4_thread_id to parent
        // ...
        // should not return
        DEBUG_printf("Hi, this is the child ...\n");
        me = l4_myself();
        DEBUG_printf("Child: Thread = '" IdFmt "'",IdStr(me));
        write(pipe_fd[1], &me, sizeof(me));
        close(pipe_fd[1]);

        DEBUG_printf("Child: told l4_threadid_t, starting server loop ...\n");
        l4x_vfs_server_loop(NULL);  
        exit(1);
    }
    else if (pid > 0) // parent
    {
        // receive l4_thread_id from child and return it
        // ...
        DEBUG_printf("Hi, this is the parent ...\n");
        read(pipe_fd[0], &me, sizeof(me));
        DEBUG_printf("Parent: Thread = '" IdFmt "'",IdStr(me));
        close(pipe_fd[0]);
        DEBUG_printf("Parent: got child's l4_threadid_t, returning it ...\n");

        return me;
    }
    else // error
    {
        perror("Error on fork: ");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return L4_INVALID_ID;
    }
}


int main(int argc, char *argv[])
{
    //LOG_init("vfs_wrap");
    names_register(L4XVFS_NAME);

    //LOG("main called");
    DEBUG_printf("main called\n");
    l4x_connection_server_loop(NULL);
    //LOG("leaving main");
    DEBUG_printf("main called\n");

    return 0; // success
}
