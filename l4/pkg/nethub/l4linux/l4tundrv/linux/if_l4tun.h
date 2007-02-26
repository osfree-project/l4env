/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef __IF_L4TUN_H
#define __IF_L4TUN_H

#ifdef __KERNEL__

#include <linux/sysctl.h>
#include <l4/sys/types.h>
#include <l4/nethub/client.h>
#include <linux/spinlock.h>

enum {
  L4TUN_IPSEC_DEV = 1,
};

struct l4tun_hub {
  char name[80];
  char flags;
  int  port;
};

struct l4tun_struct {
  char *name;
  int attached;
  struct Nh_iface iface;
  l4_threadid_t rcv, snd, hub_id;
  spinlock_t lock;
  
  unsigned decrease_interval;
  unsigned min_buffer_size;
  struct l4tun_hub hub;
  struct net_device *dev;
  struct Nh_packet_ring queue;
  struct net_device_stats stats;
  struct ctl_table conf[3];
  struct l4tun_struct *next;
};

#endif /* __KERNEL__ */

#endif /* __IF_L4TUN_H */
