#ifndef GENERIC_TS_SERVER_H
#define GENERIC_TS_SERVER_H

#include <l4/sys/types.h>

/**
 * \brief server should reply to a client
 */
void
l4ts_do_kill_reply(l4_threadid_t *src, l4_threadid_t* client);

#endif
