/** FLIPS TESTBED: SERVER */

#include "sock.h"

int main(int argc, char **argv)
{
	int s, c, err;
	struct sockaddr_in addr;
	char buf[10];
	int conn_count, pkt_count;

//	int reuse = 1;
//	struct linger linger = {1,10};

	/* get and check cmdline parameters */
	if (argc != 3) {
		printf("Usage: server <connect-count> <packet-count>\n"
		       "       max connect-count is 99\n"
		       "       max packet-count is 999\n");
		return 1;
	}

	conn_count = strtol(argv[1], NULL, 10);
	pkt_count = strtol(argv[2], NULL, 10);

	if ((conn_count > 99) || (pkt_count > 999)) {
		printf("parameters out of range\n");
		return 1;
	}

	printf("FLIPS TESTBED: SERVER  (%d connections / %d packets)\n",
	       conn_count, pkt_count);

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	if ((s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("Couldn't create socket");
//	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
//		ERR_EXIT("setsockopt(SO_REUSEADDR)");
//	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) < 0)
//		ERR_EXIT("setsockopt(SO_LINGER)");
		
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR_EXIT("Couldn't bind");
	if (listen(s, 1) < 0)
		ERR_EXIT("Couldn't listen");

	while (conn_count-- > 0) {
		int count = pkt_count;
		if ((c=accept(s, 0, 0)) < 0)
			ERR_EXIT("Problem accepting");

		while (count-- > 0) {
			memcpy(buf, "9876543210", 10);
			if (write(c, buf, 10) < 0)
				ERR_EXIT("error on write");

			memset(buf, 'X', 10);
			if ((err=read(c, buf, 10)) < 0)
				ERR_EXIT("error on read");
			else if (err == 0)
				ERR_EXIT("nothing read?");

			printf("[%02d:%03d] %.10s\n", conn_count, count, buf);
		}
	}

	shutdown(s, 2);
	return 0;
}
