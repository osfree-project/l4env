#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct ifreq;
struct in_addr;

/** UTILITY: PRINT SOCKET FLAGS */
extern void print_flags(struct ifreq *ifr);

/** UTILITY: CONFIGURE NETWORK INTERFACE */
extern void ifconfig(const char *ifname, const char *inaddr, const char *inmask);

/** FLIPS COMMAND: CONFIGURE NETWORK INTERFACE */
extern int flipscmd_ifconfig(int argc, char **argv);
