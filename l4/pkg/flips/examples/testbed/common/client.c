/** FLIPS TESTBED: CLIENT */

#include "sock.h"

int main(int argc, char **argv)
{
	int s, c;
	struct sockaddr_in addr;
	char buf[10];
	int count;

	if (argc != 2) {
		printf("Usage: client <count>\n"
		       "       max packet-count is 999\n");
		return 1;
	}

	count = strtol(argv[1], NULL, 10);

	if (count > 999) {
		printf("parameters out of range\n");
		return 1;
	}

	printf("FLIPS TESTBED: CLIENT  (%d packets)\n", count);


	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//	inet_aton("192.168.6.1", &addr.sin_addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	if ((s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("Couldn't create socket");
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR_EXIT("Couldn't connect socket");
 
	while (count-- > 0) {
		memset(buf, 'X', 10);
		if ((c=read(s, buf, 10)) < 0)
			ERR_EXIT("error on read");
		else if (c == 0)
			printf("Warning: read returns 0\n");

		printf("[%03d] %.10s\n", count, buf);

		memcpy(buf, "0123456789", 10);
		if (write(s, buf, 10) < 0)
			ERR_EXIT("error on write");
	}

	return 0;
}
