#include_next <linux/autoconf.h>

#undef  CONFIG_INET
#define CONFIG_INET 1
#undef  CONFIG_NETFILTER
#undef  CONFIG_FILTER

#undef  CONFIG_NET_SCHED

#undef  CONFIG_NETDEVICES
#define CONFIG_NETDEVICES 1
