#include <stdlib.h>
#include <errno.h>

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/basic_name_server.h>
#include <name_server-server.h>

#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/util/l4_macros.h>

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

void *CORBA_alloc(unsigned long size)
{
    void * ret;
    LOGd(_DEBUG, "CORBA_alloc: %d", size);
    LOG_flush();
    ret = malloc(size);
    LOGd(_DEBUG, "CORBA_alloc: %p", ret);
    LOG_flush();
    return ret;
}

void CORBA_free(void * ptr)
{
    LOGd(_DEBUG, "CORBA_free: %p", ptr);
    LOG_flush();
    free(ptr);
    LOGd(_DEBUG, "CORBA_free'ed: %p", ptr);
    LOG_flush();
}

object_id_t
io_basic_name_server_resolve_component(CORBA_Object _dice_corba_obj,
                                       const object_id_t *base,
                                       const char* pathname,
                                       CORBA_Environment *_dice_corba_env)
{
    return name_server_resolve(*base, pathname);
}

char*
io_basic_name_server_rev_resolve_component(CORBA_Object _dice_corba_obj,
                                           const object_id_t *dest,
                                           object_id_t *parent,
                                           CORBA_Environment *_dice_corba_env)
{
    return name_server_rev_resolve(*dest, parent);
}

l4_int32_t
io_attachable_attach_namespace_component(CORBA_Object _dice_corba_obj,
                                          volume_id_t volume_id,
                                          const char* mounted_dir,
                                          const char* mount_dir,
                                          CORBA_Environment *_dice_corba_env)
{
    /* 1.  check some conditions
     * 2a. resolve the mount_dir
     * 2b. resolve the mount_point in the destination server
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
        return 1;
    if (!is_absolute_path(mount_dir)) // check for abs. paths
        return 2;

    // 2a.
    mounted_point.volume_id = volume_id;
    mounted_point.object_id = 0; // root is 0
    server = server_for_volume(volume_id);
    LOGd(_DEBUG, "server: '" IdFmt "'", IdStr(server));
    if (l4_is_nil_id(server))
    {
        return 3; // server's volume is not registered
    }
    mounted_point = resolve(server, mounted_point, mounted_dir);
    LOGd(_DEBUG, "mounted_point = (%d.%d)",
        mounted_point.volume_id, mounted_point.object_id);

    // 2b.
    mount_point.volume_id = NAME_SERVER_VOLUME_ID;
    mount_point.object_id = root.object_id;
    mount_point = name_server_resolve(mount_point, mount_dir);
    LOGd(_DEBUG, "mount_point = (%d.%d)",
        mount_point.volume_id, mount_point.object_id);

    // 3.
    /* this may fail, e.g. if we have this volume mounted
     * somewhere already.
     */
    ret = mount_insert_entry(mount_point, mounted_point);

    return ret;
}

l4_int32_t
io_attachable_deattach_namespace_component(CORBA_Object _dice_corba_obj,
                                            const char* mount_point,
                                            CORBA_Environment *_dice_corba_env)
{
    // unmount
    return 1;  // not impl.
}

l4_int32_t
io_name_space_provider_register_volume_component(CORBA_Object _dice_corba_obj,
                                                 volume_id_t volume_id,
                                                 CORBA_Environment *_dice_corba_env)
{
    int ret;
    LOGd(_DEBUG, "register_volume: %d", volume_id);
    LOG_flush();
    ret = insert_volume_server(volume_id, *_dice_corba_obj);
    return ret; // it might fail if volume_table is full
}

l4_int32_t
io_name_space_provider_unregister_volume_component(CORBA_Object _dice_corba_obj,
                                                   volume_id_t volume_id,
                                                   CORBA_Environment *_dice_corba_env)
{
    int ret;
    ret = remove_volume_server(volume_id, *_dice_corba_obj);
    return ret; // it might fail if we don't have this pair registered
}

l4_threadid_t
io_basic_name_server_thread_for_volume_component(CORBA_Object _dice_corba_obj,
                                                 volume_id_t volume_id,
                                                 CORBA_Environment *_dice_corba_env)
{
    l4_threadid_t ret;
    LOGd(_DEBUG, "Before: io_basic_name_server_thread_for_volume_component()");
    LOG_flush();
    ret = server_for_volume(volume_id);
    LOGd(_DEBUG, "After: io_basic_name_server_thread_for_volume_component()");
    LOG_flush();
    return ret;
}

// fix me: impl. all the stuff from basic_io.idl

object_handle_t io_basic_io_open_component(CORBA_Object _dice_corba_obj,
                                           const object_id_t *object_id,
                                           l4_int32_t flags,
                                           CORBA_Environment *_dice_corba_env)
{
    int ret = 0;

    LOGd(_DEBUG, "Before: io_basic_io_open_component()");
    LOG_flush();
    // fix me: Someone could impl. better access policy here
    ret = clientstate_check_access(flags,
                                   *_dice_corba_obj,
                                   object_id->object_id);
    LOGd(_DEBUG, "In: io_basic_io_open_component()");
    LOG_flush();
    if (ret < 0)
        return ret;
    ret = clientstate_open(flags, *_dice_corba_obj, object_id->object_id);
    LOGd(_DEBUG, "After: io_basic_io_open_component()");
    LOG_flush();

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
    // fix me: currently not supported
    return -EACCES;
}

ssize_t io_rw_read_component(CORBA_Object _dice_corba_obj,
                             object_handle_t fd,
                             l4_int8_t **buf,
                             size_t *count,
                             CORBA_Environment *_dice_corba_env)
{
    // we dont like reading from our dirs
    return -EBADF;
}

ssize_t io_rw_write_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              const l4_int8_t *buf,
                              size_t *count,
                              CORBA_Environment *_dice_corba_env)
{
    // we dont like writing to our dirs
    return -EBADF;
}

off_t io_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t fd,
                                  off_t offset,
                                  l4_int32_t whence,
                                  CORBA_Environment *_dice_corba_env)
{
    off_t ret;

    ret = clientstate_lseek(fd, offset, whence, *_dice_corba_obj);

    return ret;
}

l4_int32_t io_basic_io_fsync_component(CORBA_Object _dice_corba_obj,
                                       object_handle_t fd,
                                       CORBA_Environment *_dice_corba_env)
{
    // we just do nothing, as we have not persistent storage
    return 0;
}

l4_int32_t io_basic_io_getdents_component(CORBA_Object _dice_corba_obj,
                                          object_handle_t fd,
                                          struct dirent *dirp,
                                          l4_uint32_t *count,
                                          CORBA_Environment *_dice_corba_env)
{
    int ret;

    ret = clientstate_getdents(fd, dirp, *count, *_dice_corba_obj);

    return ret;
}

int main(int argc, char *argv[])
{
    int ret;
    l4_threadid_t myself;

    LOG_init("name_ser");
    LOGd(_DEBUG, "start ...");
    init_volume_entries();
    LOGd(_DEBUG, "1 ...");
    init_dirs();
    init_mount_table();
    LOGd(_DEBUG, "2 ...");
    // register self
    myself = l4_myself();
    if ((ret = insert_volume_server(NAME_SERVER_VOLUME_ID, myself)))
    {
        LOG("Error registering self (%d)!", ret);
        abort();
    }

    if (! (ret = insert_volume_server(NAME_SERVER_VOLUME_ID, myself)))
    {
        LOG("Error registering self second time did not fail (%d)!", ret);
        abort();
    }

    // debug
    print_tree(0, &root);
    LOGd(_DEBUG, "3 ...");

    io_name_server_server_loop(NULL);
    LOGd(_DEBUG, "4 ...");
    return 0;
}
