#ifndef _ORE_LINUX_EMUL_H
#define _ORE_LINUX_EMUL_H

#include <linux/types.h>
#include <linux/skbuff.h>

#include <l4/omega0/client.h>

int init_emulation(long memsize);

extern int netif_rx(struct sk_buff *skb);

#endif /* ! _ORE_LINUX_EMUL_H */
