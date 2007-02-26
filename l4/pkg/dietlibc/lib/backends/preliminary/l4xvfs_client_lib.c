#include "l4xvfs-client.h"

#include <l4/dietlibc/l4xvfs_server.h>
#include <l4/dietlibc/l4xvfs_lib.h>

#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/sys/consts.h>
#include <l4/util/l4_macros.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

l4_threadid_t id = L4_INVALID_ID;
CORBA_Object _dice_corba_obj = &id;
CORBA_Environment _dice_corba_env = dice_default_environment;
l4_threadid_t worker_id;
CORBA_Object worker = & worker_id;

#ifdef DEBUG
#define DEBUG_LOG(...) LOG(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

int open(const char *name, int flags)
{
    DEBUG_LOG("Client, open, name = '%s', thread = '" IdFmt "'",
              name, IdStr(worker_id));
    return l4x_vfs_open_call(worker, name, flags, &errno,
                             &_dice_corba_env);
}

int close(int fd)
{
    DEBUG_LOG("Client, close, fd = '%d', thread = '" IdFmt "'",
              fd, IdStr(worker_id));
    return l4x_vfs_close_call(worker, fd, &errno, &_dice_corba_env);
}

ssize_t read(int fd, void *buf, size_t count)
{
    DEBUG_LOG("Client, read, buffer address %p", buf);
    DEBUG_LOG("(char**)&buf %p", (char**)&buf);
    DEBUG_LOG("Thread = '" IdFmt "'",IdStr(worker_id));
    return l4x_vfs_read_call(worker, fd, (l4_int8_t **)&buf,
                             &count, &errno, &_dice_corba_env);
}

ssize_t write(int fd, void *buf, size_t count)
{
    DEBUG_LOG("Client, write, buffer address %p", buf);
    DEBUG_LOG("(char**)&buf %d", (char**)&buf);
    return l4x_vfs_write_call(worker, fd, buf, &count, &errno,
                              &_dice_corba_env);
//    DEBUG_LOG("Fixme, Ron ...");
//    return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    DEBUG_LOG("Client, lseek, fd '%d', offset '%d' and whence '%d'.\n",
              fd, offset, whence);
    return l4x_vfs_lseek_call(worker, fd, offset,
                              whence, &errno, &_dice_corba_env);
}

int fstat(int fd, struct stat *buf)
{
    DEBUG_LOG("Client, fstat, fd '%d'.\n", fd);
    return l4x_vfs_fstat_call(worker, fd, buf, &errno,
                              &_dice_corba_env);
}

void l4xvfs_init(void)
{
    DEBUG_LOG("Client, init, ...\n");
    // get threadid for server
    names_waitfor_name(L4XVFS_NAME, _dice_corba_obj, 1000000);
    // init connection and get worker threadid
    DEBUG_LOG("Client, got server: " IdFmt, IdStr(*_dice_corba_obj));
    worker_id = l4x_connection_init_connection_call(_dice_corba_obj,
                                                    &_dice_corba_env);
    DEBUG_LOG("Client, got worker: " IdFmt, IdStr(worker_id));
    DEBUG_LOG("Client, got worker: " IdFmt, IdStr(*worker));
    atexit(l4xvfs_shutdown);
}

void l4xvfs_shutdown(void)
{
    DEBUG_LOG("Client, shutdown, ...\n");
    l4x_vfs_close_connection_call(worker,
                                  &_dice_corba_env);
}
