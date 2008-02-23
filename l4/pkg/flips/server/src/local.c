#include "local_s.h"

/** UTILITY: PRINT SOCKET FLAGS */
void print_flags(struct ifreq *ifr)
{
	LOG_printf("%s: ", ifr->ifr_name);
	if (ifr->ifr_flags == 0)
		LOG_printf("[NO FLAGS] ");
	if (ifr->ifr_flags & IFF_UP)
		LOG_printf("UP ");
	if (ifr->ifr_flags & IFF_BROADCAST)
		LOG_printf("BROADCAST ");
	if (ifr->ifr_flags & IFF_DEBUG)
		LOG_printf("DEBUG ");
	if (ifr->ifr_flags & IFF_LOOPBACK)
		LOG_printf("LOOPBACK ");
	if (ifr->ifr_flags & IFF_POINTOPOINT)
		LOG_printf("POINTOPOINT ");
	if (ifr->ifr_flags & IFF_NOTRAILERS)
		LOG_printf("NOTRAILERS ");
	if (ifr->ifr_flags & IFF_RUNNING)
		LOG_printf("RUNNING ");
	if (ifr->ifr_flags & IFF_NOARP)
		LOG_printf("NOARP ");
	if (ifr->ifr_flags & IFF_PROMISC)
		LOG_printf("PROMISC ");
	if (ifr->ifr_flags & IFF_ALLMULTI)
		LOG_printf("ALLMULTI ");
	if (ifr->ifr_flags & IFF_MASTER)
		LOG_printf("MASTER ");
	if (ifr->ifr_flags & IFF_SLAVE)
		LOG_printf("SLAVE ");
	if (ifr->ifr_flags & IFF_MULTICAST)
		LOG_printf("MULTICAST ");
	if (ifr->ifr_flags & IFF_DYNAMIC)
		LOG_printf("DYNAMIC ");
	LOG_printf("\n");
}

/** UTILITY: CONVERT IP ADDRESS FROM NUMBER TO STRING */
char *inet_ntoa(struct in_addr ina)
{
	static char buf[4 * sizeof "123"];
	unsigned char *ucp = (unsigned char *)&ina;

	sprintf(buf, "%d.%d.%d.%d",
	        ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
	return buf;
}

/** UTILITY: CONVERT IP ADDRESS FROM STRING TO NUMBER */
int inet_aton(const char *cp, struct in_addr *addr)
{
	unsigned int val;
	int base, n;
	char c;
	int parts[4];
	int *pp = parts;

	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, other=decimal.
		 */
		val = 0;
		base = 10;
		if (*cp == '0') {
			if (*++cp == 'x' || *cp == 'X')
				base = 16, cp++;
			else
				base = 8;
		}
		while ((c = *cp) != '\0') {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.') {
			/*
			 * Internet format:
			 *      a.b.c.d
			 *      a.b.c   (with c treated as 16-bits)
			 *      a.b     (with b treated as 24 bits)
			 */
			if (pp >= parts + 3 || val > 0xff)
				return (0);
			*pp++ = val, cp++;
		} else
			break;
	}

	/*
	 * Check for trailing characters.
	 */
	if (*cp && (!isascii(*cp) || !isspace(*cp)))
		return (0);

	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {
	case 1:                 /* a -- 32 bits */
		break;

	case 2:                 /* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:                 /* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:                 /* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);

	return (1);
}

/** UTILITY: CONFIGURE NETWORK INTERFACE */
void ifconfig(const char *ifname, const char *inaddr, const char *inmask)
{
	int s, err;
	struct ifreq ifr;
	struct sockaddr_in sa;
	struct sockaddr_in *sap;

	strncpy(ifr.ifr_name, ifname, 16);

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		ERR_EXIT("socket");

	if ((err = socket_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
	print_flags(&ifr);

	/* switch ON */
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
	if ((err = socket_ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
	if ((err = socket_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
	print_flags(&ifr);

	/* switch OFF */
	ifr.ifr_flags &= ~IFF_UP;
	if ((err = socket_ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
	if ((err = socket_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
	print_flags(&ifr);

	/* switch ON */
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
	if ((err = socket_ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCSIFFLAGS) returns %d", err);
	if ((err = socket_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFFLAGS) returns %d", err);
	print_flags(&ifr);

	/* set IFADDR */
	if (!inet_aton(inaddr, &sa.sin_addr))
		ERR_EXIT("no valid IP address %s", inaddr);
	sa.sin_family = AF_INET;
	memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

	if ((err = socket_ioctl(s, SIOCSIFADDR, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCSIFADDR) returns %d", err);
	memset(&ifr.ifr_addr, 0, sizeof(struct sockaddr));
	ifr.ifr_addr.sa_family = AF_INET;
	if ((err = socket_ioctl(s, SIOCGIFADDR, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFADDR) returns %d", err);
	sap = (struct sockaddr_in *)&ifr.ifr_addr;
	LOG_printf("%s: inet %s\n", ifr.ifr_name, inet_ntoa(sap->sin_addr));

	/* set IFNETMASK */
	if (!inet_aton(inmask, &sa.sin_addr))
		ERR_EXIT("no valid IP network mask %s", inmask);
	sa.sin_family = AF_INET;
	memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

	if ((err = socket_ioctl(s, SIOCSIFNETMASK, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCSIFNETMASK) returns %d", err);
	memset(&ifr.ifr_addr, 0, sizeof(struct sockaddr));
	ifr.ifr_addr.sa_family = AF_INET;
	if ((err = socket_ioctl(s, SIOCGIFNETMASK, &ifr)) < 0)
		ERR_EXIT("ioctl(SIOCGIFNETMASK) returns %d", err);
	sap = (struct sockaddr_in *)&ifr.ifr_addr;
	LOG_printf("%s: netmask %s\n", ifr.ifr_name, inet_ntoa(sap->sin_addr));

	socket_close(s);
}

