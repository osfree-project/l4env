#ifndef __OMEGA0_LIB_INTERNAL_H
#define __OMEGA0_LIB_INTERNAL_H

#include <l4/sys/types.h>

/* lib initialized (server found) ? */
extern int omega0_initalized;
extern l4_threadid_t omega0_management_thread;

extern int omega0_init(void);
extern int omega0_call(int lthread, omege0_request_descriptor type,
                       l4_umword_t param, l4_timeout_t timeout);
extern int omega0_open_call(int lthread, omege0_request_descriptor type,
                            l4_umword_t param, l4_timeout_t timeout,
                            l4_threadid_t*, l4_umword_t*,l4_umword_t*);
extern int omega0_call_long(int lthread, omege0_request_descriptor type,
                            l4_umword_t param, l4_threadid_t thread);

#endif
