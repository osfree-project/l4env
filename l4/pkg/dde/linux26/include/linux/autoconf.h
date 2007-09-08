/* Include Linux' contributed autoconf file */
#include_next <linux/autoconf.h>

#undef CONFIG_MODULES

/* Because of DDE SMP model */
#undef CONFIG_SMP
#define CONFIG_SMP
#undef CONFIG_NR_CPUS
#define CONFIG_NR_CPUS 16

/* Because we don't need INET support */
#undef CONFIG_INET
#undef CONFIG_XFRM
#undef CONFIG_IP_NF_IPTABLES
#undef CONFIG_IP_FIB_HASH
#undef CONFIG_NETFILTER_XT_MATCH_STATE
#undef CONFIG_INET_XFRM_MODE_TRANSPORT
#undef CONFIG_INET_XFRM_MODE_TUNNEL
#undef CONFIG_IP_NF_CONNTRACK
#undef CONFIG_TCP_CONG_BIC
#undef CONFIG_IP_NF_FILTER
#undef CONFIG_IP_NF_FTP
#undef CONFIG_IP_NF_TARGET_LOG

/* No PROC fs for us */
#undef CONFIG_PROC_FS

/* Also, no sysFS */
#undef CONFIG_SYSFS

/* We don't support hotplug */
#undef CONFIG_HOTPLUG

/* No Sysctl */
#undef CONFIG_SYSCTL

/* No power management */
#undef CONFIG_PM

/* irqs assigned statically */
#undef CONFIG_GENERIC_IRQ_PROBE