#ifndef __DIETLIBC_LIB_BACKENDS_INCLUDE_BASIC_IO_H_
#define __DIETLIBC_LIB_BACKENDS_INCLUDE_BASIC_IO_H_

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/basic_io-client.h>

#include <dirent.h>

object_handle_t basic_io_open(l4_threadid_t server,
                              object_id_t object_id,
                              l4_int32_t flags);

l4_int32_t basic_io_close(l4_threadid_t server,
                          object_handle_t object_handle);

object_handle_t basic_io_creat(l4_threadid_t server,
                               const object_id_t *parent,
                               const char* name,
                               mode_t mode);

off_t basic_io_lseek(l4_threadid_t server,
                     object_handle_t fd,
                     off_t offset,
                     l4_int32_t whence);

l4_int32_t basic_io_fsync(l4_threadid_t server,
                          object_handle_t fd);

l4_int32_t basic_io_getdents(l4_threadid_t server,
                             object_handle_t fd,
                             struct dirent *dirp,
                             l4_uint32_t count);

#endif
