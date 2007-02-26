/**
 * \file   l4vfs/name_server/examples/name_server_test/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
//#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>

#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/container_io.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/name_space_provider.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/volume_ids.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char LOG_tag[9]="n_s_test";

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
    int ret;
    object_id_t base, res_obj, root;
    char * path;
    object_handle_t handle;
    struct dirent de;
    struct dirent * temp;

    l4_sleep(1000);

    LOG("Results follow below (should be as predicted (123) == '123').");
    LOG("Volume stuff: *******************");

    ns = l4vfs_get_name_server_threadid();
    LOG("Name server thread id (8.2) '" l4util_idfmt "'", l4util_idstr(ns));

    vs = l4vfs_thread_for_volume(ns, L4VFS_NAME_SERVER_VOLUME_ID);
    LOG("Volume server for NS volume (8.2) '" l4util_idfmt "'",
        l4util_idstr(vs));

    vs = l4vfs_thread_for_volume(ns, L4VFS_NAME_SERVER_VOLUME_ID + 3);
    LOG("Volume server for other volume (0.0) '" l4util_idfmt "'",
        l4util_idstr(vs));

    root.volume_id = 5;
    root.object_id = 0;
    ret = l4vfs_register_volume(ns, l4_myself(), root);
    LOG("Register_volume(5) returned (0) %d.", ret);
    vs = l4vfs_thread_for_volume(ns, 5);
    LOG("Volume server for volume 5 (9.2)'" l4util_idfmt "'", l4util_idstr(vs));

    ret = l4vfs_register_volume(ns, l4_myself(), root);
    LOG("Register_volume(5) returned (1) %d.", ret);
    vs = l4vfs_thread_for_volume(ns, 5);
    LOG("Volume server for volume 5 (9.2) '" l4util_idfmt "'", l4util_idstr(vs));

    root.volume_id = 6;
    ret = l4vfs_register_volume(ns, l4_myself(), root);
    LOG("Register_volume(6) returned (0) %d.", ret);
    vs = l4vfs_thread_for_volume(ns, 6);
    LOG("Volume server for volume 6 (9.2) '" l4util_idfmt "'", l4util_idstr(vs));

    vs = l4vfs_thread_for_volume(ns, 7);
    LOG("Volume server for volume 7 (0.0) '" l4util_idfmt "'", l4util_idstr(vs));

    ret = l4vfs_unregister_volume(ns, l4_myself(), root.volume_id);
    LOG("Unregister_volume(6) returned (0) %d.", ret);
    vs = l4vfs_thread_for_volume(ns, 6);
    LOG("Volume server for volume 6 (0.0) '" l4util_idfmt "'", l4util_idstr(vs));

    ret = l4vfs_unregister_volume(ns, l4_myself(), root.volume_id);
    LOG("Unregister_volume(6) returned (1) %d.", ret);

    LOG("Resolve stuff: *******************");
    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = l4vfs_resolve(ns, base, "/test3");
    LOG("Resolved '/test3' (0.5): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/test3/a");
    LOG("Resolved '/test3/a' (0.6): %d.%d", res_obj.volume_id,
        res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/test3/b");
    LOG("Resolved '/test3/b' (0.7): %d.%d", res_obj.volume_id,
        res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/test3/c");
    LOG("Resolved '/test3/c' (0.8): %d.%d", res_obj.volume_id,
        res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/linux");
    LOG("Resolved '/linux' (0.1): %d.%d", res_obj.volume_id,
        res_obj.object_id);

    res_obj = l4vfs_resolve(ns, base, "linux");
    LOG("Resolved 'linux' (0.1): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "server");
    LOG("Resolved 'server' (0.2): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/test3");
    res_obj = l4vfs_resolve(ns, res_obj, "c");
    LOG("Resolved '/test3' and 'c' (0.8): %d.%d", res_obj.volume_id,
        res_obj.object_id);

    // Now we try to resolve something below a mounted file. This should not
    // work.
    res_obj = l4vfs_resolve(ns, base, "/file1/test");
    LOG("Resolved '/file1/test' (-1.-1): %d.%d", res_obj.volume_id,
        res_obj.object_id);

    LOG("Rev_resolve stuff: *******************");
    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    res_obj.object_id = 0;
    path = l4vfs_rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.0' ('/'): '%s'", path);
    res_obj.object_id = 1;
    path = l4vfs_rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.1' ('/linux'): '%s'", path);
    res_obj.object_id = 2;
    path = l4vfs_rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.2' ('/server'): '%s'", path);
    res_obj.object_id = 8;
    path = l4vfs_rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.8' ('/test3/c'): '%s'", path);

    // Now we try to rev_resolve a mounted file.
    res_obj.volume_id = STATIC_FILE_SERVER_VOLUME_ID;
    res_obj.object_id = 1;
    path = l4vfs_rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '56.1' ('/file1'): '%s'", path);

/*
name_ser|  (0)/
name_ser|     test3 (5)/
name_ser|         c (8)/
name_ser|         b (7)/
name_ser|         a (6)/
name_ser|     test2 (4)/
name_ser|     test1 (3)/
name_ser|     server (2)/
name_ser|     linux (1)/
 */

    LOG("Open/Close stuff: *******************");
    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = l4vfs_resolve(ns, base, "/");
//    LOG("*");
    handle = l4vfs_open(ns, res_obj, O_RDONLY);
    LOG("basic_io_open ('0'): '%d'", handle);
    ret = l4vfs_close(ns, handle);
    LOG("basic_io_close ('0'): '%d'", ret);

    ret = l4vfs_close(ns, handle);
    LOG("basic_io_close ('-EBADF'): '%d'", ret);
    ret = l4vfs_close(ns, handle + 4);
    LOG("basic_io_close ('-EBADF'): '%d'", ret);

    LOG("mkdir stuff: *******************");
    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    ret = l4vfs_mkdir(ns, &base, "testing_mkdir", O_RDONLY);
    LOG("container_io_mkdir(ns, 0, 'testing_mkdir', O_RDONLY) = '%d'", ret);
    LOG("Getdents stuff: *******************");
    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = l4vfs_resolve(ns, base, "/");
    handle = l4vfs_open(ns, res_obj, O_RDONLY);
//    enter_kdebug("Start tracing now !");
    ret = l4vfs_getdents(ns, handle, (l4vfs_dirent_t*)&de, sizeof(de));
    LOG("basic_io_getdents for '/' ('ca. 136'): '%d'", ret);

    LOG("Now trying to interpret the data ...");
    LOG("should be: '.', '..', 'testing_mkdir', 'test3', 'test2', "
        "'test1', 'server', 'linux'.");
    temp = &de;
    while ((char *)temp < (char *)(ret + (char *)&de))
    {
        LOG("name = '%s',\toff = '%ld',\treclen = '%d'",
            temp->d_name, temp->d_off, (int)(temp->d_reclen));
        temp = (struct dirent *)((char *)temp + temp->d_reclen);
        l4_sleep(100);
    }
    ret = l4vfs_close(ns, handle);


    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = l4vfs_resolve(ns, base, "/test3/");
    handle = l4vfs_open(ns, res_obj, O_RDONLY);
//    enter_kdebug("Start tracing now !");
    ret = l4vfs_getdents(ns, handle, (l4vfs_dirent_t*)&de, sizeof(de));
    LOG("basic_io_getdents for '/test3/' ('ca. 64'): '%d'", ret);

    LOG("Now trying to interpret the data ...");
    LOG("should be: '.', '..', 'c', 'b', 'a'.");
    temp = &de;
    while ((char *)temp < (char *)(ret + (char *)&de))
    {
        LOG("name = '%s',\toff = '%ld',\treclen = '%d'",
            temp->d_name, temp->d_off, (int)(temp->d_reclen));
        temp = (struct dirent *)((char *)temp + temp->d_reclen);
        l4_sleep(100);
    }
    ret = l4vfs_close(ns, handle);

    LOG("Attach stuff: *******************");
    ret = l4vfs_attach_namespace(ns, STATIC_FILE_SERVER_VOLUME_ID,
                                 "/", "/linux");
    LOG("attach_namespace(ns, 56, '/', '/linux') = '%d'", ret);

    base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = l4vfs_resolve(ns, base, "/");
    LOG("Resolved '/' (0.0): %d.%d",
        res_obj.volume_id, res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/linux");
    LOG("Resolved '/linux' (56.0): %d.%d",
        res_obj.volume_id, res_obj.object_id);
    res_obj = l4vfs_resolve(ns, base, "/linux/hosts");
    LOG("Resolved '/linux/hosts' (56.1): %d.%d",
        res_obj.volume_id, res_obj.object_id);

    return 0;
}
