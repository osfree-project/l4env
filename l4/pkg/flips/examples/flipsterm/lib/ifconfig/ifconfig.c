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

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "ifconfig.h"

#define ERR_EXIT(fmt, args...) \
	do {                       \
		printf("Error: ");     \
		printf(fmt, ## args);  \
		printf("\n");          \
		return;                \
	} while (0)

/** UTILITY: PRINT SOCKET FLAGS */
void print_flags(struct ifreq *ifr)
{
  printf("%s: ", ifr->ifr_name);
  if (ifr->ifr_flags == 0)
    printf("[NO FLAGS] ");
  if (ifr->ifr_flags & IFF_UP)
    printf("UP ");
  if (ifr->ifr_flags & IFF_BROADCAST)
    printf("BROADCAST ");
  if (ifr->ifr_flags & IFF_DEBUG)
    printf("DEBUG ");
  if (ifr->ifr_flags & IFF_LOOPBACK)
    printf("LOOPBACK ");
  if (ifr->ifr_flags & IFF_POINTOPOINT)
    printf("POINTOPOINT ");
  if (ifr->ifr_flags & IFF_NOTRAILERS)
    printf("NOTRAILERS ");
  if (ifr->ifr_flags & IFF_RUNNING)
    printf("RUNNING ");
  if (ifr->ifr_flags & IFF_NOARP)
    printf("NOARP ");
  if (ifr->ifr_flags & IFF_PROMISC)
    printf("PROMISC ");
  if (ifr->ifr_flags & IFF_ALLMULTI)
    printf("ALLMULTI ");
  if (ifr->ifr_flags & IFF_MASTER)
    printf("MASTER ");
  if (ifr->ifr_flags & IFF_SLAVE)
    printf("SLAVE ");
  if (ifr->ifr_flags & IFF_MULTICAST)
    printf("MULTICAST ");
  if (ifr->ifr_flags & IFF_DYNAMIC)
    printf("DYNAMIC ");
  printf("\n");
}

/** UTILITY: CONFIGURE NETWORK INTERFACE */
void ifconfig(const char *ifname, const char *inaddr, const char *inmask)
{
  int s, err;
  struct ifreq ifr;
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
  printf("%s: inet %s\n", ifr.ifr_name, inet_ntoa(sap->sin_addr));
  
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
  printf("%s: netmask %s\n", ifr.ifr_name, inet_ntoa(sap->sin_addr));

  close(s);
}


