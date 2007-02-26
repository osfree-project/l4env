//#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>

#include <l4/dietlibc/attachable.h>
#include <l4/dietlibc/basic_io.h>
#include <l4/dietlibc/basic_name_server.h>
#include <l4/dietlibc/name_server.h>
#include <l4/dietlibc/name_space_provider.h>

#include <l4/dietlibc/io-types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    object_id_t base, res_obj;
    char * path;
    object_handle_t handle;
    struct dirent de;
    struct dirent * temp;

    l4_sleep(1000);

    LOG_init("n_s_test");

    LOG("Results follow below (should be as predicted (123) == '123').");
    LOG("Volume stuff: *******************");

    ns = get_name_server_threadid();
    LOG("Name server thread id (8.2) '" IdFmt "'", IdStr(ns));

    vs = thread_for_volume(ns, NAME_SERVER_VOLUME_ID);
    LOG("Volume server for NS volume (8.2) '" IdFmt "'", IdStr(vs));

    vs = thread_for_volume(ns, NAME_SERVER_VOLUME_ID + 3);
    LOG("Volume server for other volume (0.0) '" IdFmt "'", IdStr(vs));

    ret = register_volume(ns, 5);
    LOG("Register_volume(5) returned (0) %d.", ret);
    vs = thread_for_volume(ns, 5);
    LOG("Volume server for volume 5 (9.2)'" IdFmt "'", IdStr(vs));

    ret = register_volume(ns, 5);
    LOG("Register_volume(5) returned (1) %d.", ret);
    vs = thread_for_volume(ns, 5);
    LOG("Volume server for volume 5 (9.2) '" IdFmt "'", IdStr(vs));

    ret = register_volume(ns, 6);
    LOG("Register_volume(6) returned (0) %d.", ret);
    vs = thread_for_volume(ns, 6);
    LOG("Volume server for volume 6 (9.2) '" IdFmt "'", IdStr(vs));

    vs = thread_for_volume(ns, 7);
    LOG("Volume server for volume 7 (0.0) '" IdFmt "'", IdStr(vs));

    ret = unregister_volume(ns, 6);
    LOG("Unregister_volume(6) returned (0) %d.", ret);
    vs = thread_for_volume(ns, 6);
    LOG("Volume server for volume 6 (0.0) '" IdFmt "'", IdStr(vs));

    ret = unregister_volume(ns, 6);
    LOG("Unregister_volume(6) returned (1) %d.", ret);

    LOG("Resolve stuff: *******************");
    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = resolve(ns, base, "/test3");
    LOG("Resolved '/test3' (0.5): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/test3/a");
    LOG("Resolved '/test3/a' (0.6): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/test3/b");
    LOG("Resolved '/test3/b' (0.7): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/test3/c");
    LOG("Resolved '/test3/c' (0.8): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/linux");
    LOG("Resolved '/linux' (0.1): %d.%d", res_obj.volume_id, res_obj.object_id);

    res_obj = resolve(ns, base, "linux");
    LOG("Resolved 'linux' (0.1): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "server");
    LOG("Resolved 'server' (0.2): %d.%d", res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/test3");
    res_obj = resolve(ns, res_obj, "c");
    LOG("Resolved '/test3' and 'c' (0.8): %d.%d", res_obj.volume_id, res_obj.object_id);

    LOG("Rev_resolve stuff: *******************");
    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj.volume_id = NAME_SERVER_VOLUME_ID;
    res_obj.object_id = 0;
    path = rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.0' ('/'): '%s'", path);
    res_obj.object_id = 1;
    path = rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.1' ('/linux'): '%s'", path);
    res_obj.object_id = 2;
    path = rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.2' ('/server'): '%s'", path);
    res_obj.object_id = 8;
    path = rev_resolve(ns, res_obj, &base);
    LOG("Rev_resolved '0.8' ('/test3/c'): '%s'", path);

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
    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = resolve(ns, base, "/");
//    LOG("*");
    handle = basic_io_open(ns, res_obj, O_RDONLY);
    LOG("basic_io_open ('0'): '%d'", handle);
    ret = basic_io_close(ns, handle);
    LOG("basic_io_close ('0'): '%d'", ret);

    ret = basic_io_close(ns, handle);
    LOG("basic_io_close ('-EBADF'): '%d'", ret);
    ret = basic_io_close(ns, handle + 4);
    LOG("basic_io_close ('-EBADF'): '%d'", ret);

    LOG("Getdents stuff: *******************");
    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = resolve(ns, base, "/");
    handle = basic_io_open(ns, res_obj, O_RDONLY);
//    enter_kdebug("Start tracing now !");
    ret = basic_io_getdents(ns, handle, &de, sizeof(de));
    LOG("basic_io_getdents for '/' ('ca. 112'): '%d'", ret);

    LOG("Now trying to interpret the data ...");
    LOG("should be: '.', '..', 'test3', 'test2', 'test1', 'server', 'linux'.");
    temp = &de;
    while ((char *)temp < (char *)(ret + (char *)&de))
    {
        LOG("name = '%s',\toff = '%d',\treclen = '%d'",
            temp->d_name, temp->d_off, (int)(temp->d_reclen));
        temp = (struct dirent *)((char *)temp + temp->d_reclen);
    }
    ret = basic_io_close(ns, handle);


    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = resolve(ns, base, "/test3/");
    handle = basic_io_open(ns, res_obj, O_RDONLY);
//    enter_kdebug("Start tracing now !");
    ret = basic_io_getdents(ns, handle, &de, sizeof(de));
    LOG("basic_io_getdents for '/test3/' ('ca. 64'): '%d'", ret);

    LOG("Now trying to interpret the data ...");
    LOG("should be: '.', '..', 'c', 'b', 'a'.");
    temp = &de;
    while ((char *)temp < (char *)(ret + (char *)&de))
    {
        LOG("name = '%s',\toff = '%d',\treclen = '%d'",
            temp->d_name, temp->d_off, (int)(temp->d_reclen));
        temp = (struct dirent *)((char *)temp + temp->d_reclen);
    }
    ret = basic_io_close(ns, handle);

    LOG("Attach stuff: *******************");
    ret = attach_namespace(ns, 56, "/", "/linux");
    LOG("attach_namespace(ns, 56, '/', '/linux') = '%d'", ret);

    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
    res_obj = resolve(ns, base, "/");
    LOG("Resolved '/' (0.0): %d.%d",
        res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/linux");
    LOG("Resolved '/linux' (56.0): %d.%d",
        res_obj.volume_id, res_obj.object_id);
    res_obj = resolve(ns, base, "/linux/hosts");
    LOG("Resolved '/linux/hosts' (56.1): %d.%d",
        res_obj.volume_id, res_obj.object_id);

    return 0;
}
