/**
 * \file   l4vfs/static_file_server/server/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>

#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/name_space_provider.h>
#include <l4/l4vfs/name_server.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/object_server-server.h>

#include <l4/static_file_server/static_file_server.h>

#include "basic_io.h"
#include "resolve.h"
#include "state.h"

char LOG_tag[9] = STATIC_FILE_SERVER_NAME;

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

object_handle_t
l4vfs_basic_io_open_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4_int32_t flags,
                              CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    if (object_id->volume_id != STATIC_FILE_SERVER_VOLUME_ID)
        return -ENOENT;
    ret = clientstate_open(flags, *_dice_corba_obj, object_id->object_id);

    return ret;
}

l4_int32_t l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                           object_handle_t object_handle,
                                           CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_close(object_handle, *_dice_corba_obj);

    return ret;
}

l4vfs_ssize_t
l4vfs_common_io_read_component (CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                char **buf,
                                l4vfs_size_t *count,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_read(fd, *buf, *count);
    *_dice_reply = DICE_REPLY;

    return ret;
}

l4vfs_ssize_t
l4vfs_common_io_write_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t handle,
                                 const char *buf,
                                 l4vfs_size_t *count,
                                 short *_dice_reply,
                                 CORBA_Server_Environment *_dice_corba_env)
{
    return -EROFS;
}

l4vfs_off_t l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                                           object_handle_t fd,
                                           l4vfs_off_t offset,
                                           l4_int32_t whence,
                                           CORBA_Server_Environment *_dice_corba_env)
{
    int ret;
    
    LOGd(_DEBUG, "fd = %d, offset = %d, whence = %d", fd, offset, whence);
    ret = clientstate_seek(fd, offset, whence);
    LOGd(_DEBUG, "ret = %d", ret);

    return ret;
}

l4_int32_t l4vfs_basic_io_fsync_component(CORBA_Object _dice_corba_obj,
                                          object_handle_t fd,
                                          CORBA_Server_Environment *_dice_corba_env)
{
    // just do nothing
    return 0;
}

object_id_t
l4vfs_basic_name_server_resolve_component(CORBA_Object _dice_corba_obj,
                                          const object_id_t *base,
                                          const char* pathname,
                                          CORBA_Server_Environment *_dice_corba_env)
{
    return internal_resolve(*base, pathname);
}

l4_threadid_t l4vfs_basic_name_server_thread_for_volume_component(
    CORBA_Object _dice_corba_obj, volume_id_t volume_id,
    CORBA_Server_Environment *_dice_corba_env)
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
    CORBA_Server_Environment env = dice_default_server_environment;
    object_id_t root_id;

    names_register(STATIC_FILE_SERVER_NAME);

    LOGd(_DEBUG, "main called");
    state_init();
    LOGd(_DEBUG, "after: state_init");

    l4_sleep(500);
    ns = l4vfs_get_name_server_threadid();
    LOGd(_DEBUG, "got name server thread_id");
    root_id.volume_id = STATIC_FILE_SERVER_VOLUME_ID;
    root_id.object_id = 0;
    l4vfs_register_volume(ns, l4_myself(), root_id);
    LOGd(_DEBUG, "registered");

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    l4vfs_object_server_server_loop(&env);
    LOGd(_DEBUG, "leaving main");

    return 0; // success
}
