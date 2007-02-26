#include <l4/dietlibc/basic_io.h>

#include <l4/dietlibc/basic_io-client.h>

#include <l4/sys/types.h>

#include <dirent.h>

static CORBA_Environment _dice_corba_env = dice_default_environment;

object_handle_t basic_io_open(l4_threadid_t server,
                              object_id_t object_id,
                              l4_int32_t flags)
{
    return io_basic_io_open_call(&server,
                                 &object_id,
                                 flags,
                                 &_dice_corba_env);
}

l4_int32_t basic_io_close(l4_threadid_t server,
                          object_handle_t object_handle)
{
    return io_basic_io_close_call(&server,
                                  object_handle,
                                  &_dice_corba_env);
}

object_handle_t basic_io_creat(l4_threadid_t server,
                               const object_id_t *parent,
                               const char* name,
                               mode_t mode)
{
    return io_basic_io_creat_call(&server,
                                  parent,
                                  name,
                                  mode,
                                  &_dice_corba_env);
}

off_t basic_io_lseek(l4_threadid_t server,
                     object_handle_t fd,
                     off_t offset,
                     l4_int32_t whence)
{
    return io_basic_io_lseek_call(&server,
                                  fd,
                                  offset,
                                  whence,
                                  &_dice_corba_env);
}

l4_int32_t basic_io_fsync(l4_threadid_t server,
                          object_handle_t fd)
{
    return io_basic_io_fsync_call(&server,
                                  fd,
                                  &_dice_corba_env);
}

l4_int32_t basic_io_getdents(l4_threadid_t server,
                             object_handle_t fd,
                             struct dirent *dirp,
                             l4_uint32_t count)
{
    return io_basic_io_getdents_call(&server,
                                     fd,
                                     dirp,
                                     &count,
                                     &_dice_corba_env);
}
