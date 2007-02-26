#ifndef __OMEGA0_LIB_INTERNAL_H
#define __OMEGA0_LIB_INTERNAL_H

#include <l4/sys/types.h>

/* lib initialized (server found) ? */
extern int omega0_initalized;

extern int omega0_init(void);
extern int omega0_call(int lthread, omege0_request_descriptor type,
                       l4_umword_t param);
extern int omega0_call_long(int lthread, omege0_request_descriptor type,
                            l4_umword_t param, l4_threadid_t thread);

#endif
