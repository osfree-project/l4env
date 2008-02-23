#include_next <linux/proc_fs.h>

/* XXX prevent compilation error produced by unconditionally using
 * "proc_net_stat" in net/ipv4/route.c *grmpf*
 */
#ifndef CONFIG_PROC_FS
extern struct proc_dir_entry *proc_net_stat;
#endif
