#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <l4/dietlibc/sfs_lib.h>

#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>

#include <l4/dietlibc/attachable.h>
#include <l4/dietlibc/basic_io.h>
#include <l4/dietlibc/basic_name_server.h>
#include <l4/dietlibc/name_space_provider.h>
#include <l4/dietlibc/name_server.h>

#include <l4/dietlibc/io-types.h>
#include <object_server-server.h>

#include "basic_io.h"
#include "resolve.h"
#include "state.h"

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt)
{
    free(prt);
}

object_handle_t io_basic_io_open_component(CORBA_Object _dice_corba_obj,
                                           const object_id_t *object_id,
                                           l4_int32_t flags,
                                           CORBA_Environment *_dice_corba_env)
{
    int ret;

    if (object_id->volume_id != STATIC_FILE_SERVER_VOLUME_ID)
        return -ENOENT;
    ret = clientstate_open(flags, *_dice_corba_obj, object_id->object_id);

    return ret;
}

l4_int32_t io_basic_io_close_component(CORBA_Object _dice_corba_obj,
                                       object_handle_t object_handle,
                                       CORBA_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_close(object_handle, *_dice_corba_obj);

    return ret;
}


object_handle_t io_basic_io_creat_component(CORBA_Object _dice_corba_obj,
                                            const object_id_t *parent,
                                            const char* name,
                                            mode_t mode,
                                            CORBA_Environment *_dice_corba_env)
{
    return -EROFS;
}

ssize_t io_rw_read_component(CORBA_Object _dice_corba_obj,
                             object_handle_t fd,
                             l4_int8_t **buf,
                             size_t *count,
                             CORBA_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_read(fd, *buf, *count);

    return ret;
}

ssize_t io_rw_write_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              const l4_int8_t *buf,
                              size_t *count,
                              CORBA_Environment *_dice_corba_env)
{
    return -EROFS;
}

off_t io_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t fd,
                                  off_t offset,
                                  l4_int32_t whence,
                                  CORBA_Environment *_dice_corba_env)
{
    int ret;
    
    ret = clientstate_seek(fd, offset, whence);

    return ret;
}

l4_int32_t io_basic_io_fsync_component(CORBA_Object _dice_corba_obj,
                                       object_handle_t fd,
                                       CORBA_Environment *_dice_corba_env)
{
    // just do nothing
    return 0;
}


l4_int32_t io_basic_io_getdents_component(CORBA_Object _dice_corba_obj,
                                          object_handle_t fd,
                                          struct dirent *dirp,
                                          l4_uint32_t *count,
                                          CORBA_Environment *_dice_corba_env)
{
    // fix me: so something useful here
    return -EBADF;
}

object_id_t
io_basic_name_server_resolve_component(CORBA_Object _dice_corba_obj,
                                       const object_id_t *base,
                                       const char* pathname,
                                       CORBA_Environment *_dice_corba_env)
{
    return internal_resolve(*base, pathname);
}

char*
io_basic_name_server_rev_resolve_component(CORBA_Object _dice_corba_obj,
                                           const object_id_t *dest,
                                           object_id_t *parent,
                                           CORBA_Environment *_dice_corba_env)
{
    return internal_rev_resolve(*dest, parent);
}

l4_threadid_t
io_basic_name_server_thread_for_volume_component(CORBA_Object _dice_corba_obj,
                                                 volume_id_t volume_id,
                                                 CORBA_Environment *_dice_corba_env)
{
    if (volume_id == STATIC_FILE_SERVER_VOLUME_ID)
    {
        return l4_myself();
    }
    return L4_INVALID_ID;
}

int main(int argc, char *argv[])
{
    l4_threadid_t ns;

    names_register(SFS_NAME);
    LOG_init("stat_fs");

    LOGd(_DEBUG, "main called");
    state_init();
    LOGd(_DEBUG, "after: state_init");

    l4_sleep(500);
    ns = get_name_server_threadid();
    LOGd(_DEBUG, "got name server thread_id");
    register_volume(ns, STATIC_FILE_SERVER_VOLUME_ID);
    LOGd(_DEBUG, "registered");

    io_object_server_server_loop(NULL);
    LOGd(_DEBUG, "leaving main");

    return 0; // success
}
