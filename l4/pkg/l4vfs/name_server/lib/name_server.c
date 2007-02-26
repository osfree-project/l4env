/**
 * \file   l4vfs/name_server/lib/name_server.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/name_server.h>

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/rmgr/librmgr.h>

#include <l4/log/l4log.h>

static int initialized = 0;
static l4_threadid_t name_server_thread_id;

/* Shamelessly stolen from names' lib and extended.
 * This functions computes the name server's service thread id.
 * At the end of this function we check for the server thread's existence.
 * That is, this should be guaranteed after a successful return.
 */
l4_threadid_t l4vfs_get_name_server_threadid()
{
    int error, i;
    l4_umword_t ignore;
    l4_timeout_t l4s = L4_IPC_BOTH_TIMEOUT_0;  // don't wait to long
    l4_msgdope_t result;
    int leave = 0;

    if (! initialized)
    {
        if (! rmgr_init())
        {
            LOG("RMGR not initialized, name_server not found!");
            return L4_INVALID_ID;
        }

        error = rmgr_get_task_id("name_server", &name_server_thread_id);
        if (error)
        {
            LOG("Error looking up 'name_server' at RMGR!");
            return L4_INVALID_ID;
        }

        if (! l4_is_invalid_id(name_server_thread_id))
            initialized = 1;
        else
        {
            LOG("'name_server' not known at RMGR!");
            return name_server_thread_id;
        }
    }

    // fast fix for l4env: running thread is 2
    // fixme: Is there a better way to find the running thread?
    //        This must probably be adapted forL4V4.
    name_server_thread_id.id.lthread = 2;

    // here we check whether the computed thread exists by receiving a
    // (bogus) IPC with timeout 0 and checking the error code
    i = 0;
    do
    {
        error = l4_ipc_receive(name_server_thread_id, L4_IPC_SHORT_MSG,
                               &ignore, &ignore, l4s, &result);
        switch(error)
        {
        case 0:    // we received something, strange, we should not ... anyway
            leave = 1;
            break;
        case L4_IPC_ENOT_EXISTENT:  // thread not yet existing, wait and retry
            i++;
            if (i > 10)  // something went probably wrong, be verbose about it
            {
                i = 0;
                LOG("Still waiting for 'name_server' (" l4util_idfmt ").",
                     l4util_idstr(name_server_thread_id));
            }
            l4_sleep(100);
            break;
        case L4_IPC_RETIMEOUT:      // default case, thread exists but did not
            leave = 1;              // sent anything, ok!
            break;
        default:   // hmm, I don't know what to do now
            //LOG("default");
            break;
        }
    } while (! leave);

    return name_server_thread_id;
}
