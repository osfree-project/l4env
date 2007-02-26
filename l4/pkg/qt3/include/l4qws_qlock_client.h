#ifndef __L4QWS_QLOCK_CLIENT_H
#define __L4QWS_QLOCK_CLIENT_H

#include <l4/qt3/l4qws_qlock-client.h>
#include <l4/qt3/l4qws_defs.h>
#include <l4/qt3/l4qws_key.h>

extern CORBA_Environment l4qws_dice_qlock_env;
extern l4_threadid_t     l4qws_qlock_server;

#ifdef __cplusplus
extern "C" {
#endif

/* **************************************************************** */

void l4qws_qlock_client_init(void);

/* **************************************************************** */

#ifdef __cplusplus
}
#endif

#endif /* __L4QWS_QLOCK_CLIENT_H*/
