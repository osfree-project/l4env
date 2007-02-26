#ifndef __DIETLIBC_LIB_BACKENDS_INCLUDE_RW_H_
#define __DIETLIBC_LIB_BACKENDS_INCLUDE_RW_H_

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/rw-client.h>

ssize_t basic_io_read(l4_threadid_t server,
                      object_handle_t fd,
                      l4_int8_t **buf,
                      size_t *count);

ssize_t basic_io_write(l4_threadid_t server,
                       object_handle_t fd,
                       const l4_int8_t *buf,
                       size_t *count);

#endif
