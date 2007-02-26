/****************************************************************************
 * ORe transmit lock implementation.                                        *
 *                                                                          * 
 * As ORe is multi-threaded, it is possible that several clients issue      *
 * send requests to the same device in parrallel. The device driver itself  *
 * is not necessarily thread-aware, therefore we need to secure access to   *
 * the driver's hard_start_xmit() routine by a lock.                        *
 * The ORe xmit lock manages a list of device names and keeps exactly one   *
 * lock for each device.                                                    *
 *                                                                          *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                              *
 * 2005-11-17                                                               *
 *                                                              			*
 * (c) 2005 - 2007 Technische Universitaet Dresden							*
 * This file is part of DROPS, which is distributed under the   			*
 * terms of the GNU General Public License 2. Please see the    			*
 * COPYING file for details.                                    			*
 ****************************************************************************/

#include "ore-local.h"

//TODO: comments

#define MAX_LOCKS 10

static struct
{
    char * device_name;
    l4lock_t xmit_lock;
} xmit_locks[MAX_LOCKS];

static int xmit_initialized = 0;

static void xmit_initialize(void)
{
    int i;
    for (i=0; i<MAX_LOCKS; i++)
        xmit_locks[i].device_name = NULL;
}

int xmit_lock(char *dev)
{
    int i;
    
    if (!xmit_initialized)
        xmit_initialize();
    
    for (i=0; i<MAX_LOCKS; i++)
    {
        if (xmit_locks[i].device_name == NULL)
            continue;

        if (!strcmp(dev, xmit_locks[i].device_name))
        {
            l4lock_lock(&xmit_locks[i].xmit_lock);
            return 0;
        }
    }

    return -1;
}

int xmit_unlock(char *dev)
{
    int i;

    if (!xmit_initialized)
        xmit_initialize();
    
    for (i=0; i<MAX_LOCKS; i++)
    {
        if (xmit_locks[i].device_name == NULL)
            continue;

        if (!strcmp(dev, xmit_locks[i].device_name))
        {
            l4lock_unlock(&xmit_locks[i].xmit_lock);
            return 0;
        }
    }
    return 0;
}

int xmit_lock_add(char *dev)
{
    int i=0;

    if (!xmit_initialized)
        xmit_initialize();
    
    while (xmit_locks[i].device_name != NULL)
        i++;
    
    if (i < MAX_LOCKS)
    {
        xmit_locks[i].device_name = dev;
        xmit_locks[i].xmit_lock   = L4LOCK_UNLOCKED;
        return 0;
    }

    return -1;
}

int xmit_lock_remove(char *dev)
{
    int i=0;

    if (!xmit_initialized)
        xmit_initialize();
    
    while (strcmp(xmit_locks[i].device_name, dev))
        i++;

    if (i < MAX_LOCKS)
    {
        xmit_locks[i].device_name = NULL;
        return 0;
    }

    return -1;
}

