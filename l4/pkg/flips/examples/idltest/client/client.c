/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/flips/libflips.h>

l4_ssize_t l4libc_heapsize = 500*1024;
l4_threadid_t srv;
char LOG_tag[9] = "idlt-clt";


static void original_socket_test(const char* srv_addr, unsigned int port,
                                 int count)
{
	int s, c;
	struct sockaddr_in addr;
	char buf[10];

	printf("<ORIGINAL SOCKET TEST>\n");

	if (srv_addr)
		inet_aton(srv_addr, &addr.sin_addr);
	else
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	printf("try to open socket...\n");
	while ((s=flips_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("cannot create socket, keep trying...\n");
		l4thread_sleep(1000);
	}
	printf("got socket descriptor %d\n",s);

	if (flips_connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		printf("couldn't connect socket\n");

	while (count--) {
		int size;
		memset(buf, 'X', 10);
		size = 10;
		if ((c=flips_read(s, buf, size)) < 0)
			printf("error on read\n");
		else if (c == 0)
			printf("read returns 0\n");
		
		printf("%.10s\n", buf);
		
		memcpy(buf, "0123456789", 10);
		if (flips_write(s, buf, 10) < 0)
			printf("error on write\n");
		l4thread_sleep(1000);
	}
}

static void proc_test(void)
{
	static char procbuf[1024];
	int len = 512;

	printf("<PROC TEST>\n");

	printf("test proc_read\n");
	flips_proc_read("/proc/net/dev", &procbuf[0], 0, len);
	printf("*** content of /proc/net/dev: ***\n%s\n*** end of content ***\n",
	       &procbuf[0]);
}

int main(int argc, char **argv)
{
	/* the tests */
	int do_proc_test = 0;
	int do_original_socket_test = 1;

	/* some options */
	char *dest = NULL;  /* default destination address is LOOPBACK */
	unsigned int port = 9999;
	int count = 3;

	/* XXX parse cmdline and update settings */

	/* XXX server may be remote! */
	printf("wait for idltest-server at names\n");
	{
		l4_threadid_t dummy;
		while (names_waitfor_name("idltest", &dummy, 2500) == 0) {
			printf("wait for idltest-server\n");
		}
	}
	printf("idltest-server is up.\n");

	/* proc test */
	if (do_proc_test) proc_test();

	/* original socket test */
	/* XXX */ dest = "192.168.6.10";
	dest = NULL;
	if (do_original_socket_test)
		original_socket_test(dest, port, count);

	return 0;
}

