/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#include <l4/qt3/l4qws_qlock_client.h>

/*
 * ************************************************************************
 */

DICE_DECLARE_ENV(l4qws_dice_qlock_env);
l4_threadid_t     l4qws_qlock_server   = L4_INVALID_ID_INIT;

/*
 * ************************************************************************
 */

void l4qws_qlock_client_init(void) {

  if (names_waitfor_name("qws-qlock", &l4qws_qlock_server, 5000) == 0)
    LOG_Error("names-query for 'qws-qlock' failed\n");
}
