
/** UTILITY: PRINT SOCKET FLAGS */
extern void print_flags(struct ifreq *ifr);

/** UTILITY: CONVERT IP ADDRESS FROM NUMBER TO STRING */
extern char* inet_ntoa(struct in_addr ina);

/** UTILITY: CONVERT IP ADDRESS FROM STRING TO NUMBER */
extern int inet_aton(const char *cp, struct in_addr *addr);

/** UTILITY: CONFIGURE NETWORK INTERFACE */
extern void ifconfig(const char *ifname, const char *inaddr, const char *inmask, const char *gateway);
