/*
 * \brief   Minimalistic Network Interface configuration tool
 * \date    2003-08-07
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This code comes from the local.c of the FLIPS server. Now it
 * works as a separate client that communicates to FLIPS via
 * the libflips (IDL interface).
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#include "ifconfig.h"

#define IFCONFIG_DEBUG_VERBOSE 0

#define ERR_EXIT(fmt, args...) \
	do {                       \
		LOG_Error(fmt, ## args);  \
		return;                \
	} while (0)

#ifdef USE_UCLIBC
#define IFF_DYNAMIC 0x8000
#endif

/*** UTILITY: PRINT SOCKET FLAGS ***
 *
 * If you change anything, check size of the string buffer!
 */
void print_flags(struct ifreq *ifr)
{
	static char buffer[192];

	if (IFCONFIG_DEBUG_VERBOSE)
		return;

	sprintf(buffer, "FLAGS for %s: ", ifr->ifr_name);
	if (ifr->ifr_flags == 0)
		strcat(buffer, "[NO FLAGS] ");
	else {
		if (ifr->ifr_flags & IFF_UP)
			strcat(buffer, "UP ");
		if (ifr->ifr_flags & IFF_BROADCAST)
			strcat(buffer, "BROADCAST ");
		if (ifr->ifr_flags & IFF_DEBUG)
			strcat(buffer, "DEBUG ");
		if (ifr->ifr_flags & IFF_LOOPBACK)
			strcat(buffer, "LOOPBACK ");
		if (ifr->ifr_flags & IFF_POINTOPOINT)
			strcat(buffer, "POINTOPOINT ");
		if (ifr->ifr_flags & IFF_NOTRAILERS)
			strcat(buffer, "NOTRAILERS ");
		if (ifr->ifr_flags & IFF_RUNNING)
			strcat(buffer, "RUNNING ");
		if (ifr->ifr_flags & IFF_NOARP)
			strcat(buffer, "NOARP ");
		if (ifr->ifr_flags & IFF_PROMISC)
			strcat(buffer, "PROMISC ");
		if (ifr->ifr_flags & IFF_ALLMULTI)
			strcat(buffer, "ALLMULTI ");
		if (ifr->ifr_flags & IFF_MASTER)
			strcat(buffer, "MASTER ");
		if (ifr->ifr_flags & IFF_SLAVE)
			strcat(buffer, "SLAVE ");
		if (ifr->ifr_flags & IFF_MULTICAST)
			strcat(buffer, "MULTICAST ");
		if (ifr->ifr_flags & IFF_PORTSEL)
			strcat(buffer, "PORTSEL ");
		if (ifr->ifr_flags & IFF_AUTOMEDIA)
			strcat(buffer, "AUTOMEDIA ");
		if (ifr->ifr_flags & IFF_DYNAMIC)
			strcat(buffer, "DYNAMIC ");
	}
	LOG("%s", buffer);
}

/** UTILITY: CONFIGURE NETWORK INTERFACE */
void ifconfig(const char *ifname, const char *inaddr, const char *inmask, const char* gateway)
{
  int s, err;
  struct ifreq ifr;
  struct rtentry rt;
  struct sockaddr_in sa;
  struct sockaddr_in *sap;

  strncpy(ifr.ifr_name, ifname, 16);

  if ((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    ERR_EXIT("socket");

  if ((err=ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
  print_flags(&ifr); 

  /* switch ON */
  ifr.ifr_flags |= (IFF_UP|IFF_RUNNING);
  if ((err=ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
  if ((err=ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
  print_flags(&ifr);

  /* switch OFF */
  ifr.ifr_flags &= ~IFF_UP;
  if ((err=ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
  if ((err=ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
  print_flags(&ifr);

  /* switch ON */
  ifr.ifr_flags |= (IFF_UP|IFF_RUNNING);
  if ((err=ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
  if ((err=ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
  print_flags(&ifr);

  /* set IFADDR */
  if (!inet_aton(inaddr, &sa.sin_addr))
    ERR_EXIT("no valid IP address %s", inaddr);
  sa.sin_family = AF_INET;
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

  if ((err=ioctl(s, SIOCSIFADDR, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCSIFADDR) returns %d", err);
  memset(&ifr.ifr_addr, 0, sizeof(struct sockaddr));
  ifr.ifr_addr.sa_family = AF_INET;
  if ((err=ioctl(s, SIOCGIFADDR, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFADDR) returns %d", err);
  sap = (struct sockaddr_in *)&ifr.ifr_addr;
  LOG("%s: inet %s", ifr.ifr_name, inet_ntoa(sap->sin_addr));

  /* set IFNETMASK */
  if (!inet_aton(inmask, &sa.sin_addr))
    ERR_EXIT("no valid IP network mask %s", inmask);
  sa.sin_family = AF_INET;
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

  if ((err=ioctl(s, SIOCSIFNETMASK, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCSIFNETMASK) returns %d", err);
  memset(&ifr.ifr_addr, 0, sizeof(struct sockaddr));
  ifr.ifr_addr.sa_family = AF_INET;
  if ((err=ioctl(s, SIOCGIFNETMASK, &ifr)) < 0)
    ERR_EXIT("ioctl(SIOCGIFNETMASK) returns %d", err);
  sap = (struct sockaddr_in *)&ifr.ifr_addr;
  LOG("%s: netmask %s", ifr.ifr_name, inet_ntoa(sap->sin_addr));

  /* set gateway */
  if (gateway != NULL)
  { 
    memset (&rt, '\0', sizeof (rt));
    if (!inet_aton(gateway, &((struct sockaddr_in *) &rt.rt_gateway)->sin_addr))
      ERR_EXIT("no valid gateway IP address %s", gateway);

    rt.rt_dst.sa_family     = AF_INET;
    rt.rt_gateway.sa_family = AF_INET;
    rt.rt_flags             = RTF_UP | RTF_GATEWAY;
    if ((err=ioctl(s, SIOCADDRT, &rt)) < 0)
      ERR_EXIT("ioctl(SIOCADDRT) returns %d %d", err, sizeof(rt));

    sap = (struct sockaddr_in *)&rt.rt_gateway;
    LOG("%s: gateway %s", ifr.ifr_name, inet_ntoa(sap->sin_addr));
  }else
    LOG("%s: gateway none", ifr.ifr_name);

  close(s);
}
