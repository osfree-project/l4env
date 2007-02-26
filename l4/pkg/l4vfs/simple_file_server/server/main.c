/**
 * \file   l4vfs/simple_file_server/server/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <l4/simple_file_server/simple_file_server.h>
#include <l4/l4vfs/select_notify-server.h>
#include <l4/l4vfs/select_listener.h>

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
#include <l4/dm_generic/types.h>
#include <l4/l4vfs/mmap_object_server-server.h>

#include "simple_file_server-server.h"
#include "basic_io.h"
#include "resolve.h"
#include "state.h"
#include "dirs.h"

char LOG_tag[9] = SIMPLE_FILE_SERVER_NAME;

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

volume_id_t my_volume_id;


object_handle_t
l4vfs_basic_io_open_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4_int32_t flags,
                              CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    if (object_id->volume_id != my_volume_id)
        return -ENOENT;
    ret = clientstate_open(flags, *_dice_corba_obj, object_id->object_id);

    return ret;
}


l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t object_handle,
                                CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_close(object_handle, *_dice_corba_obj);

    return ret;
}


l4_int32_t
l4vfs_basic_io_stat_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4vfs_stat_t *buf,
                              CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    if (object_id->volume_id != my_volume_id)
        return -ENOENT;
    ret = clientstate_stat(buf, *_dice_corba_obj,
                           object_id->object_id);

    return ret;
}


l4_int32_t
l4vfs_basic_io_access_component(CORBA_Object _dice_corba_obj,
                                const object_id_t *object_id,
                                l4_int32_t mode,
                                CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    if (object_id->volume_id != my_volume_id)
        return -ENOENT;
    ret = clientstate_access(mode, *_dice_corba_obj, object_id->object_id);

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

    return ret;
}


l4vfs_ssize_t
l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                const char *buf,
                                l4vfs_size_t *count,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_write(fd, buf, *count);

    return ret;
}


l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_seek(fd, offset, whence);

    return ret;
}

l4_int32_t
l4vfs_basic_io_fsync_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               CORBA_Server_Environment *_dice_corba_env)
{
    // just do nothing
    return 0;
}


int l4vfs_basic_io_getdents_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      l4vfs_dirent_t **dirp,
                                      unsigned int *count,
                                      CORBA_Server_Environment *_dice_corba_env)
{   int ret;

    ret = clientstate_getdents(fd,*dirp,*count,*_dice_corba_obj);

    return ret;

}


l4_int32_t l4vfs_mmap_msync_component(CORBA_Object _dice_corba_obj,
                                      const l4dm_dataspace_t *ds,
                                      l4_addr_t start,
                                      l4vfs_size_t length,
                                      l4_int32_t flags,
                                      CORBA_Server_Environment *_dice_corba_env)

{   int ret;

    ret = clientstate_msync(ds,start,length,flags);

    return ret;
}


int l4vfs_mmap_mmap_component(CORBA_Object _dice_corba_obj,
                              l4dm_dataspace_t *ds,
                              l4vfs_size_t length,
                              l4_int32_t prot,
                              l4_int32_t flags,
                              object_handle_t fd,
                              l4vfs_off_t offset,
                              CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_mmap(ds,length,prot,flags,fd,offset);

    return ret;
}


l4_int32_t
l4vfs_mmap_munmap_component(CORBA_Object _dice_corba_obj,
                            const l4dm_dataspace_t *ds,
                            l4_addr_t start,
                            l4vfs_size_t length,
                            CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_munmap(ds, start, length);

    return ret;
}


void
l4vfs_select_notify_request_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      l4_int32_t mode,
                                      const l4_threadid_t *notif_tid,
                                      CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_select_notify(&fd, &mode);

    LOGd(_DEBUG,"ret: %d, fd %d",ret, fd);

    if (ret == 0)
    {
        l4vfs_select_listener_send_notification((l4_threadid_t) *notif_tid,
                                                fd, mode);
    }
}


void
l4vfs_select_notify_clear_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t fd,
                                    l4_int32_t mode,
                                    const l4_threadid_t *notif_tid,
                                    CORBA_Server_Environment *_dice_corba_env)
{
   /* nothing todo, because simple file server is always ready for read and
    * does not need to save notify messages */
}


object_id_t l4vfs_basic_name_server_resolve_component(
    CORBA_Object _dice_corba_obj,
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
    if (volume_id == my_volume_id)
    {
        return l4_myself();
    }
    return L4_INVALID_ID;
}


void usage(const char * prog);
void usage(const char * prog)
{
    LOG("Usage: %s [-v <id>]\n"
        "    -v, --volume_id .. The volume_id to use for this instance\n"
        "\n"
        "Example: %s -v 12\n", prog, prog);
}

int main(int argc, char *argv[])
{
    l4_threadid_t ns;
    int i, ret;
    CORBA_Server_Environment env = dice_default_server_environment;
    object_id_t root_id;

    int optionid, option;
    const struct option long_options[] =
        {
            { "volume-id",   1, NULL, 'v'},
            { 0, 0, 0, 0}
        };

    // parse my parameters
    my_volume_id = SIMPLE_FILE_SERVER_VOLUME_ID;
    do
    {
        option = getopt_long(argc, argv, "v:", long_options, &optionid);
        switch (option)
        {
        case 'v':
            my_volume_id = atoi(optarg);
            LOGd(_DEBUG,"volume-id = %d", my_volume_id);
            if (my_volume_id <= 0)
            {
                LOG("Illegal volume_id specified (%d)", my_volume_id);
                usage(argv[0]);
                exit(1);
            }
            break;
        case -1:  // abort case
            break;
        default:
            LOG("error  - unknown option %c", option);
            usage(argv[0]);
            exit(1);
            break;
        }
    } while (option != -1);

    names_register(SIMPLE_FILE_SERVER_NAME);
    state_init();

    l4_sleep(500);
    ns = l4vfs_get_name_server_threadid();
    LOGd(_DEBUG, "got name server thread_id");
    root_id.volume_id = my_volume_id;
    root_id.object_id = 0;

    for (i = 0; i < 3; i++)
    {
        ret = l4vfs_register_volume(ns, l4_myself(), root_id);
        // register ok --> done
        if (ret == 0)
            break;
        else
            LOG("error registering: %d", ret);
        l4_sleep(500);
    }

    if (i > 2)
    {
        LOG("tried to register 3 times: failed");
        exit(1);
    }

    LOGd(_DEBUG, "registered");

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    simple_file_server_server_loop(&env);

    return 0; // success
}
