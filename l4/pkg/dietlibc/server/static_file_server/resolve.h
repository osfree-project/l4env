#ifndef __DIETLIBC_SERVER_STATIC_FILE_SERVER_RESOLVE_H_
#define __DIETLIBC_SERVER_STATIC_FILE_SERVER_RESOLVE_H_

#include <l4/dietlibc/io-types.h>
#include <l4/sys/types.h>

object_id_t internal_resolve(object_id_t base,
                             const char * pathname);

char *  internal_rev_resolve(object_id_t dest,
                             object_id_t *parent);

#endif
