#ifndef LOCAL_H
#define LOCAL_H

/* Linux */
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>

/* L4 */
#include <l4/util/macros.h>
#include "config.h"

/*** LIBLINUX ***/
extern int liblinux_socket_init(void);  /* from socket.c */
extern int liblinux_softirq_init(void); /* from softirq.c */
extern int liblinux_timer_init(void);   /* from timer.c */
extern int liblinux_sysctl_init(void);  /* from sysctl.c */
extern int liblinux_lo_init(struct net_device *dev);
                                        /* from lo.c */
extern int liblinux_socket_init(void);  /* from socket.c */

/*** LINUX ***/
extern int netlink_proto_init(void);    /* from netlink/af_netlink.c */
extern void rtnetlink_init(void);       /* from core/rtnetlink.c */
extern void net_dev_init(void);         /* from core/dev.c */

/*** MISC ***/
void liblinux_net_rx_action(struct softirq_action *);
void liblinux_skb_stats(struct sk_buff *skb, int rx);
void liblinux_check_netif_rx(struct net_device *dev, int value);

//#include <l4/thread/thread.h>
//l4thread_t liblinux_sofirq_nr2id(int nr);

#endif

