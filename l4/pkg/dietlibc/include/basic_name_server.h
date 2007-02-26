#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_BASIC_NAME_SERVER_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_BASIC_NAME_SERVER_H_

#include <l4/dietlibc/io-types.h>
#include <l4/sys/types.h>

object_id_t resolve(l4_threadid_t server,
                    object_id_t base,
                    const char * pathname);

char *  rev_resolve(l4_threadid_t server,
                    object_id_t dest,
                    object_id_t *parent);

l4_threadid_t thread_for_volume(l4_threadid_t server,
                                volume_id_t volume_id);

#endif
