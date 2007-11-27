/**
 * \file   l4vfs/name_server/server/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/errno.h>
#include <l4/l4vfs/types.h>
#include <name_server-server.h>

#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/util/l4_macros.h>
#include <l4/names/libnames.h>

#include "volumes.h"
#include "dirs.h"
#include "resolve.h"
#include "pathnames.h"
#include "mount_table.h"
#include "clientstate.h"

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

object_id_t
l4vfs_basic_name_server_resolve_component(
    CORBA_Object _dice_corba_obj, const object_id_t *base,
    const char* pathname, CORBA_Server_Environment *_dice_corba_env)
{
    return name_server_resolve(*base, pathname);
}

char*
l4vfs_basic_name_server_rev_resolve_component(
    CORBA_Object _dice_corba_obj, const object_id_t *dest, object_id_t *parent,
    CORBA_Server_Environment *_dice_corba_env)
{
    char * ret;

    ret = name_server_rev_resolve(*dest, parent);
    // tell dice to free ret after using it
    dice_set_ptr(_dice_corba_env, ret);

    return ret;
}

l4_int32_t
l4vfs_extendable_attach_namespace_component(
    CORBA_Object _dice_corba_obj, volume_id_t volume_id,
    const char* mounted_dir, const char* mount_dir,
    CORBA_Server_Environment *_dice_corba_env)
{
    /* 1.  check some conditions
     * 2a. resolve the mount_point in the destination server
     * 2b. resolve the mount_dir
     * 3.  insert pair into mount table
     *
     *     of course we should check errors everywhere
     */

    object_id_t mount_point, mounted_point;
    l4_threadid_t server;
    int ret;

    LOGd(_DEBUG, "attach_namespace(%d, '%s', '%s')",
         volume_id, mounted_dir, mount_dir);
    LOG_flush();

    // 1.
    if (!is_absolute_path(mounted_dir)) // check for abs. paths
        return L4VFS_ENOT_ABS_PATH;
    if (!is_absolute_path(mount_dir))   // check for abs. paths
        return L4VFS_ENOT_ABS_PATH;

    // 2a.
    mounted_point.volume_id = volume_id;
    mounted_point.object_id = root_for_volume(volume_id);
    if (mounted_point.object_id == L4VFS_ILLEGAL_OBJECT_ID)
    {
        return L4VFS_ERESOLVE;          // no valid root entry found
    }

    server = server_for_volume(volume_id);
    LOGd(_DEBUG, "server: '" l4util_idfmt "'", l4util_idstr(server));
    if (l4_is_invalid_id(server))
    {
        return L4VFS_EVOL_NOT_REG;      // server's volume is not registered
    }
    mounted_point = l4vfs_resolve(server, mounted_point, mounted_dir);
    LOGd(_DEBUG, "mounted_point = (%d.%d)",
        mounted_point.volume_id, mounted_point.object_id);
    if (mounted_point.volume_id == L4VFS_ILLEGAL_VOLUME_ID)
    {
        return L4VFS_ERESOLVE;          // could not resolve mounted point
    }

    // 2b.
    mount_point.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    mount_point.object_id = root.spec.dir.object_id;
    mount_point = name_server_resolve(mount_point, mount_dir);
    LOGd(_DEBUG, "mount_point = (%d.%d)",
        mount_point.volume_id, mount_point.object_id);
    if (mount_point.volume_id == L4VFS_ILLEGAL_VOLUME_ID)
    {
        return L4VFS_ERESOLVE;          // could not resolve mount point
    }

    // check for other errors
    if ((ret = mount_check_boundary_conditions(mount_point, mounted_point)))
    {
        return EBUSY;
    }

    // 3.
    /* this may fail, e.g. if we have this volume mounted
     * somewhere already.
     */
    ret = mount_insert_entry(mount_point, mounted_point);
    if (ret)
        return ENOMEM;  // mount table full
    else
        return 0;
}

l4_int32_t
l4vfs_extendable_detach_namespace_component(
    CORBA_Object _dice_corba_obj, const char* mount_point,
    CORBA_Server_Environment *_dice_corba_env)
{
    // unmount
    return 1;  // not impl.
}

l4_int32_t
l4vfs_name_space_provider_register_volume_component(
    CORBA_Object _dice_corba_obj, const_CORBA_Object server,
    const object_id_t *root_id, CORBA_Server_Environment *_dice_corba_env)
{
    int ret;
    LOGd(_DEBUG, "register_volume: %d.%d",
         root_id->volume_id, root_id->object_id);
    LOGd(_DEBUG, "server: "l4util_idfmt, l4util_idstr(*server));
    LOG_flush();
    ret = insert_volume_server(*root_id, *server);
    return ret; // it might fail if volume_table is full
}

l4_int32_t
l4vfs_name_space_provider_unregister_volume_component(
    CORBA_Object _dice_corba_obj, const_CORBA_Object server,
    volume_id_t volume_id, CORBA_Server_Environment *_dice_corba_env)
{
    int ret;
    ret = remove_volume_server(volume_id, *server);
    return ret; // it might fail if we don't have this pair registered
}

l4_threadid_t
l4vfs_basic_name_server_thread_for_volume_component(
    CORBA_Object _dice_corba_obj, volume_id_t volume_id,
    CORBA_Server_Environment *_dice_corba_env)
{
    l4_threadid_t ret;
    LOGd(_DEBUG,
         "Before: l4vfs_basic_name_server_thread_for_volume_component()");
    LOG_flush();
    ret = server_for_volume(volume_id);
    LOGd(_DEBUG,
         "After: l4vfs_basic_name_server_thread_for_volume_component()");
    LOG_flush();
    return ret;
}

// fix me: impl. all the stuff from basic_io.idl

object_handle_t
l4vfs_basic_io_open_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4_int32_t flags,
                              CORBA_Server_Environment *_dice_corba_env)
{
    int ret = 0;

    LOGd(_DEBUG, "Before: l4vfs_basic_io_open_component()");
    LOG_flush();
    // fix me: Someone could impl. better access policy here
    ret = clientstate_check_access(flags,
                                   *_dice_corba_obj,
                                   object_id->object_id);
    LOGd(_DEBUG, "In: l4vfs_basic_io_open_component()");
    LOG_flush();
    if (ret < 0)
        return ret;
    ret = clientstate_open(flags, *_dice_corba_obj, object_id->object_id);
    LOGd(_DEBUG, "After: l4vfs_basic_io_open_component()");
    LOG_flush();

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

object_handle_t
l4vfs_basic_io_creat_component(CORBA_Object _dice_corba_obj,
                               const object_id_t *parent,
                               const char* name,
                               l4_int32_t flags,
                               l4vfs_mode_t mode,
                               CORBA_Server_Environment *_dice_corba_env)
{
    // we dont support files in the name-server managed part of the namespace
    return -EACCES;
}

int
l4vfs_container_io_mkdir_component(CORBA_Object _dice_corba_obj,
                                   const object_id_t *parent,
                                   const char* name,
                                   l4vfs_mode_t mode,
                                   CORBA_Server_Environment *_dice_corba_env)
{
    obj_t *p;
    int ret;

    // check some errors ...
    // check volume_id
    if (parent->volume_id != L4VFS_NAME_SERVER_VOLUME_ID)
        return -ENOENT;

    // check if local object_id can be resolved
    p = map_get_dir(parent->object_id);
    if (p == NULL)
        return -ENOENT;

    ret = dir_add_child_dir(p, name);
    if (ret == 2 || ret == 3)
        return -EEXIST;
    else if (ret == 1)
        return -EPERM; // illegal character ???

    return 0;
}

l4vfs_ssize_t
l4vfs_common_io_read_component (CORBA_Object _dice_corba_obj,
                                object_handle_t handle,
                                char **buf,
                                l4vfs_size_t *count,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
    // we dont like reading from our dirs
    *_dice_reply = DICE_REPLY;
    return -EBADF;
}

l4vfs_ssize_t
l4vfs_common_io_write_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t handle,
                                 const char *buf,
                                 l4vfs_size_t *count,
                                 short *_dice_reply,
                                 CORBA_Server_Environment *_dice_corba_env)
{
    // we dont like writing to our dirs
    return -EBADF;
}

l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
{
    off_t ret;

    ret = clientstate_lseek(fd, offset, whence, *_dice_corba_obj);

    return ret;
}

l4_int32_t
l4vfs_basic_io_fsync_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               CORBA_Server_Environment *_dice_corba_env)
{
    // we just do nothing, as we have no persistent storage
    return 0;
}

l4_int32_t
l4vfs_basic_io_getdents_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t fd,
                                  l4vfs_dirent_t **dirp,
                                  l4_uint32_t *count,
                                  CORBA_Server_Environment *_dice_corba_env)
{
    int ret;

    LOGd(_DEBUG, "Before: getdents(%d, %p, %d)",
         fd, *dirp, *count);
    LOG_flush();
    ret = clientstate_getdents(fd, *dirp, *count, *_dice_corba_obj);
    LOGd(_DEBUG, "After: ret = %d)", ret);
    LOG_flush();

    return ret;
}

l4_int32_t
l4vfs_basic_io_stat_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4vfs_stat_t *buf,
                              CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if object exists
     * 2. fill data structure
     * 3. return it
     */
    obj_t * obj;

    obj = map_get_dir(object_id->object_id);
    if (obj == NULL)
        return -ENOENT;

    buf->st_dev   = L4VFS_NAME_SERVER_VOLUME_ID;
    buf->st_ino   = object_id->object_id;
    buf->st_mode  = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    buf->st_nlink = 1;
    buf->st_size  = 0;  // useless but defined ...
    // fixme: other fields are undefined for now ...

    return 0;
}

l4_int32_t
l4vfs_name_server_attach_object_component(CORBA_Object _dice_corba_obj,
                                          const object_id_t *base,
                                          const char* name,
                                          const object_id_t *object,
                                          CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if base is valid, owned by us, and a directory.
     * 2. check if object.object_id is valid (>= 0).
     * 3. check if object.volume_id is valid and registered.
     * 4. check if name already exists in it.
     * 5. create entry and return.
     */

    // fixme: test this
    // fixme: map to good errnos
    obj_t * obj;
    int ret;

    // 1.
    if (base->volume_id != L4VFS_NAME_SERVER_VOLUME_ID)
        return -1000;
    if ((base->object_id < 0) || (base->object_id >= NAME_SERVER_MAX_DIRS))
        return -1001;
    if ((obj = map_get_dir(base->object_id)) == NULL)
        return -1002;
    if (! dir_is_dir(obj))
        return -1003;

    // 2.
    if (object->object_id < 0)
        return -1004;

    // 3.
    if (index_for_volume(object->volume_id) < 0)
        return -1005;

    // 4., 5.
    ret = dir_add_child_file(obj, name, *object);
    if (ret != 0)
        return -1006;

    return 0;
}

void usage(void);
void usage(void)
{
    LOG_printf("usage: name_server [-e] [-p]\n"
               "\n"
               "  -e,--examples .. create an exemplary root name space\n"
               "                   with some dirs and files\n"
               "  -p,--print ..... print current root name space\n"
        );
}

int main(int argc, char *argv[])
{
    int ret, create_examples = 0, do_print_tree = 0;
    l4_threadid_t myself;
    CORBA_Server_Environment env = dice_default_server_environment;
    object_id_t root_id;

    // parse parameters
    {
        int optionid;
        int option = 0;
        const struct option long_options[] =
            {
                { "examples", 0, NULL, 'e'},
                { "print",    0, NULL, 'p'},
                { 0, 0, 0, 0}
            };

        do
        {
            option = getopt_long(argc, argv, "ep", long_options, &optionid);
            switch (option)
            {
            case 'e':
                create_examples = 1;
                break;
            case 'p':
                do_print_tree = 1;
                break;
            case -1:  // exit case
                break;
            default:
                LOG("error  - unknown option %c", option);
                usage();
                return 1;
            }
        } while (option != -1);

    }

    if (! names_register("name_server"))
    {
        LOG("Error registering at names!");
        abort();
    }

    LOGd(_DEBUG, "start ...");
    init_volume_entries();
    LOGd(_DEBUG, "1 ...");
    init_dirs(create_examples);
    init_mount_table();
    LOGd(_DEBUG, "2 ...");

    // register self
    myself = l4_myself();
    root_id.object_id = L4VFS_ROOT_OBJECT_ID;
    root_id.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    if ((ret = insert_volume_server(root_id, myself)))
    {
        LOG("Error registering self (%d)!", ret);
        abort();
    }

    /*
    // debug
    if (! (ret = insert_volume_server(root_id, myself)))
    {
        LOG("Error registering self second time did not fail (%d)!", ret);
        abort();
    }
    */

    // debug
    if (do_print_tree)
        print_tree(0, &root);
    LOGd(_DEBUG, "3 ...");

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    l4vfs_name_server_server_loop(&env);
    LOGd(_DEBUG, "4 ...");
    return 0;
}
