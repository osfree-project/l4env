#include <l4/events/events.h>
#include "ore-local.h"

/* (c) 2005 - 2007 Technische Universitaet Dresden			 
 * This file is part of DROPS, which is distributed under the
 * terms of the GNU General Public License 2. Please see the 
 * COPYING file for details.                                 
 */

/* Thread to care for events. Listens to exit events and closes
 * established connections if a client goes down.
 */
void handle_events(void *argp)
{
#ifdef CONFIG_ORE_EVENTS
    l4events_ch_t event_ch  = L4EVENTS_EXIT_CHANNEL;
    l4events_nr_t event_nr  = L4EVENTS_NO_NR;
    l4events_event_t event;
    int ret;
    
    l4thread_started(0);

    ret = l4events_init();
    LOGd(ORE_DEBUG_EVENTS, "initialized events: %d (=1?)", ret);

    if (!ret)
    {
        LOG_Error("Events server not found.");
        l4thread_exit();
    }
    
    l4events_register(event_ch, ORE_EVENTS_THREAD_PRIORITY);
    LOGd(ORE_DEBUG_EVENTS, "Events thread started.");

    while(1)
    {
        l4_threadid_t thread;
        int ret;
        int handle;

        LOGd(ORE_DEBUG_EVENTS, "receiving event...");
        ret = l4events_give_ack_and_receive(&event_ch, &event,
                &event_nr, L4_IPC_NEVER, L4EVENTS_RECV_ACK);
        if (ret != L4EVENTS_OK)
        {
            LOG_Error("got bad event: %d", ret);
            continue;
        }
        LOGd(ORE_DEBUG_EVENTS, "received event: %d", ret);

        thread = *(l4_threadid_t*)event.str;
        LOGd(ORE_DEBUG_EVENTS, l4util_idfmt" has exited.", l4util_idstr(thread));
        
        handle = find_channel_for_threadid(thread, 0);
        LOGd(ORE_DEBUG_EVENTS, "client %d has terminated.", handle);
        while (handle >= 0)
        {
            LOGd(ORE_DEBUG_EVENTS, "freeing connection with handle %d", handle);
            free_connection(handle);
            handle = find_channel_for_threadid(thread, handle);
        }
    }
#endif
};
