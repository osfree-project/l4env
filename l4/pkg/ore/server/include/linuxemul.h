/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef _ORE_LINUX_EMUL_H
#define _ORE_LINUX_EMUL_H

#include <linux/types.h>
#include <linux/skbuff.h>

#include <l4/omega0/client.h>

int init_emulation(long memsize);

extern int netif_rx(struct sk_buff *skb);

#endif /* ! _ORE_LINUX_EMUL_H */
