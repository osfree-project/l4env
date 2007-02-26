#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/dde_linux/dde.h>
#include <l4/socket_linux/socket_linux.h>

/* XXX never ever mix Oskit and Linux NET
 * I was looking for a nasty sockaddr bug for hours - Linux has _no_
 * sa_len member.
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/

#include <linux/socket.h>
#include <linux/types.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <net/ip.h>
#include <ctype.h>

/* Local */
#include "liblinux.h"
#include "liblxdrv.h"

#include <stdlib.h>

/***************************/
/*** INTERNAL INTERFACES ***/
/***************************/

extern void single_thread(void);
extern void multi_thread(void);
extern void notify_thread(void);

/******************/
/*** UTIL STUFF ***/
/******************/

#define ERR_EXIT(fmt, args...) \
  do                           \
    {                          \
      LOG_Error(fmt, ## args); \
      exit(1);                 \
    } while (0)

/** UTILITY: PRINT SOCKET FLAGS */
extern void print_flags(struct ifreq *ifr);

/** UTILITY: CONVERT IP ADDRESS FROM NUMBER TO STRING */
extern char* inet_ntoa(struct in_addr ina);

/** UTILITY: CONVERT IP ADDRESS FROM STRING TO NUMBER */
extern int inet_aton(const char *cp, struct in_addr *addr);

/** UTILITY: CONFIGURE NETWORK INTERFACE */
extern void ifconfig(const char *ifname, const char *inaddr, const char *inmask);
