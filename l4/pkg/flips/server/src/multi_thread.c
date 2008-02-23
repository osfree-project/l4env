/* Multi-threaded socket example */

#include "local_s.h"

/*** CLIENT ***/

static void client(void *p)
{
	int s, err, i;
	char buf[10];
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("socket(): %d", s);

	LOG("client is up and connecting...");
	if ((err = connect(s, (struct sockaddr *)&addr, sizeof(addr))) < 0)
		ERR_EXIT("connect(%d): %d", s, err);

	i = 0;
	for (;;) {
		memset(buf, 'X', 10);
		if ((err = socket_read(s, buf, 10)) < 0)
			ERR_EXIT("read(%d): %d", s, err);
		else if (err == 0)
			ERR_EXIT("read returns 0");

		LOG_printf("[%05d] %.10s\n", ++i, buf);
		//l4thread_sleep(50);

		memcpy(buf, "0123456789", 10);
		if ((err = socket_write(s, buf, 10)) < 0)
			ERR_EXIT("write(%d): %d", s, err);
	}
}

/*** SERVER ***/

static void server(void *p)
{
	int s, c, err, i;
	char buf[10];
	struct sockaddr_in addr;
//  int reuse = 1;
//  struct linger linger = {1,10};

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	l4dde_process_add_worker();

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("Couldn't create socket");
//  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
//    ERR_EXIT("setsockopt(SO_REUSEADDR)");
//  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) < 0)
//    ERR_EXIT("setsockopt(SO_LINGER)");

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR_EXIT("Couldn't bind");
	if ((err = listen(s, 1)) < 0)
		ERR_EXIT("listen(%d): %d", s, err);

	/* we're up - notify client */
	l4thread_started(NULL);

	LOG("server is up and accepting...");
	if ((c = accept(s, 0, 0)) < 0)
		ERR_EXIT("accept(%d): %d", s, c);

	i = 0;
	for (;;) {
		memcpy(buf, "9876543210", 10);
		if ((err = socket_write(c, buf, 10)) < 0)
			ERR_EXIT("write(%d): %d", c, err);

		memset(buf, 'X', 10);
		if ((err = socket_read(c, buf, 10)) < 0)
			ERR_EXIT("read(%d): %d", c, err);
		else if (c == 0)
			ERR_EXIT("nothing read?");

		LOG_printf("[%05d] %.10s\n", ++i, buf);
	}
}

#include <linux/netdevice.h>

void multi_thread()
{
	int id;

	Kdebug("MULTI THREAD");

	/* let's rock */
	LOG("spawning server");
	id = l4thread_create_named((l4thread_fn_t) server,
	                           ".server",
	                           NULL, L4THREAD_CREATE_SYNC);
	if (id < 0)
		ERR_EXIT("l4thread_create: %d", id);

	LOG("starting client");
	client(NULL);
}

