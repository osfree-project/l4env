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
char LOG_tag[9] = "server";

int main(int argc, char **argv) {
	int s, c, err, i;
	struct sockaddr_in addr;
	char buf[10];

	printf("Server(main): ask for name \"ifconfig\"\n");
	while (names_waitfor_name("ifconfig", &srv, 2500) == 0) {
		printf("Server(main): \"ifconfig\" not available, keep trying...\n");
	}
	printf("Server(main): found \"ifconfig\" at Names.\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);
	printf("try to open socket\n");
	if ((s=flips_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		printf("Couldn't create socket\n");
	{
		int addrlen = sizeof(struct sockaddr);
		printf("addrlen = %d\n",addrlen);
		if (flips_bind(s, (struct sockaddr *)&addr, addrlen) < 0)
			printf("Couldn't bind\n");
	}
	printf("bound to socket %d, call listen\n",s);
	if ((err=flips_listen(s, 1)) < 0)
		printf("listen(%d): %d\n", s, err);

	printf("server is up and accepting...\n");

	if (!names_register("idltest")) {
		printf("Error: Cannot register at nameserver\n");
		return -1;
	}

	printf("idltest-server: I am up!\n");
	{
		if ((c=flips_accept(s, NULL, NULL)) < 0)
			printf("accept(%d): %d\n", s, c);
	}
	printf("accept passed.\n");
	i = 0;
	for (;;)
	{
		int buflen;

		memcpy(buf, "9876543210", 10);
		if ((err=flips_write(c, buf, 10)) < 0)
			printf("write(%d): %d\n", c, err);

		memset(buf, 'X', 10);
		buflen=10;
		if ((err=flips_read(c, buf, buflen)) < 0)
			printf("read(%d): %d\n", c, err);
		else if (c == 0)
			printf("nothing read?\n");

		printf("[%05d] %.10s\n", ++i, buf);
	}
	return 0;
}

