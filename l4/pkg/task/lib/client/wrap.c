/******************************************************************************
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2005 - 2007 Technische Universitaet Dresden                            *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 *                                                                            *
 * Wrapper functions implementing the old-style generic_ts stuff that is not  *
 * needed with the taskserverlib layout.                                      *
 ******************************************************************************/
#include <l4/generic_ts/generic_ts.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/rmgr/librmgr.h>

#include "tsclient.h"

l4_threadid_t l4ts_server_id = L4_NIL_ID;

/** Use l4_nchief to determine the next chief on the path to RMGR. This can
 *  be either RMGR itself or another parent which indicates that we haven't
 *  been started by RMGR.
 */
int l4ts_connect(void)
{
	int n __attribute__((unused));

	/* First, we try to get our parent from the L4Env infopage. */
	l4ts_server_id = l4env_get_parent();
	if (!l4_is_nil_id(l4ts_server_id)) {
		DEBUG_MSG("found task server: "l4util_idfmt" in infopage.", l4util_idstr(l4ts_server_id));
		return 0;
	}

	/* No need to worry here. We just know that loader is not our
	 * parent. Now we probe for our chief.
	 * If our chief is RMGR, the RMGR service thread will serve as our
	 * parent. Else, we use the chief task ID and make an assumption about
	 * the task server thread's ID.
	 */
	n = l4_nchief(rmgr_service_id(), &l4ts_server_id);
	if (l4_tasknum_equal(rmgr_service_id(), l4ts_server_id))
		l4ts_server_id = rmgr_service_id();
	else
		l4ts_server_id.id.lthread = TASKSERVER_PARENT_TID;

	DEBUG_MSG("Assuming parent taskserver at "l4util_idfmt, l4util_idstr(l4ts_server_id));

	return 0;
}

l4_threadid_t l4ts_server(void)
{
  return l4ts_server_id;
}

int l4ts_taskno_to_taskid(l4_uint32_t taskno, l4_taskid_t *taskid)
{
	LOG_Enter();
	return -1;
}


