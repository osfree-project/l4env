#include <l4/dietlibc/rw.h>

#include <l4/dietlibc/rw-client.h>

#include <l4/sys/types.h>

static CORBA_Environment _dice_corba_env = dice_default_environment;

ssize_t basic_io_read(l4_threadid_t server,
                      object_handle_t fd,
                      l4_int8_t **buf,
                      size_t *count)
{
    return io_rw_read_call(&server,
                           fd,
                           buf,
                           count,
                           &_dice_corba_env);
}

ssize_t basic_io_write(l4_threadid_t server,
                       object_handle_t fd,
                       const l4_int8_t *buf,
                       size_t *count)
{
    return io_rw_write_call(&server,
                            fd,
                            buf,
                            count,
                            &_dice_corba_env);
}
