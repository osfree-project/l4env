#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_RESOLVE_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_RESOLVE_H_

#include <l4/dietlibc/io-types.h>

#define IO_PATH_SEPARATOR '/'
#define IO_PATH_PARENT    ".."
#define IO_PATH_IDENTITY  '.'

object_id_t name_server_resolve(object_id_t base, const char * pathname);
char * name_server_rev_resolve(object_id_t dest, object_id_t * parent);
object_id_t local_resolve(object_id_t base, const char * dirname);
char * local_rev_resolve(object_id_t dest, object_id_t * parent);

#endif
