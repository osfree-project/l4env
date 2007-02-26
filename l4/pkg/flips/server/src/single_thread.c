/* Single-threaded socket example */

#include "local.h"

int sock_listen,                /* server listen socket */
 sock_srv,                      /* server socket */
 sock_clt;                      /* client socket */
#define BUFLEN 60               /* buffer length */
char buf_srv[BUFLEN],           /* server buffer */
 buf_clt[BUFLEN];               /* client buffer */
char *data_srv =
	"123456789012345678901234567890123456789012345678901234567890";
char *data_clt =
	"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";


/*** CLIENT ***/

void client_init(void)
{
	int err;
	struct sockaddr_in addr;

	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	if ((sock_clt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("client socket: %d", sock_clt);
	LOG("client is up and connecting...");
	if ((err =
	     connect(sock_clt, (struct sockaddr *)&addr, sizeof(addr))) < 0)
		ERR_EXIT("client connect(%d): %d", sock_clt, err);
}

/*** SERVER ***/

int emul_accept(void *arg)
{
	l4dde_process_add_worker();

	/* we're up - notify client */
	l4thread_started(NULL);

	LOG("server is up and accepting...");
	if ((sock_srv = accept(sock_listen, 0, 0)) < 0)
		ERR_EXIT("server accept(%d): %d", sock_listen, sock_srv);
	return 0;
}

void server_init(void)
{
	int id, err;
	struct sockaddr_in addr;
//  int reuse = 1;
//  struct linger linger = {1,10};

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	if ((sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("server socket: %d", sock_listen);
//  err = setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR,
	                     //                   &reuse, sizeof(reuse));
//  if (err < 0)
//    ERR_EXIT("server setsockopt(SO_REUSEADDR): %d", err);
//  err = setsockopt(sock_listen, SOL_SOCKET, SO_LINGER,
	                     //                   &linger, sizeof(linger));
//  if (err < 0)
//    ERR_EXIT("server setsockopt(SO_LINGER): %d", err);

	if ((err =
	     bind(sock_listen, (struct sockaddr *)&addr, sizeof(addr))) < 0)
		ERR_EXIT("server bind(%d): %d", sock_listen, err);
	if ((err = listen(sock_listen, 1)) < 0)
		ERR_EXIT("server listen(%d): %d", sock_listen, err);

	/* emulate accepting server */
	id = l4thread_create((l4thread_fn_t) emul_accept,
	                      NULL, L4THREAD_CREATE_SYNC);
	if (id < 0)
		ERR_EXIT("l4thread_create: %d", id);
}

#include <linux/netdevice.h>

void single_thread()
{
	int err, i;

	if (l4thread_set_prio(l4thread_myself(), 0x20) < 0)
		ERR_EXIT("l4thread_set_prio");
	Kdebug("SINGLE THREAD");

	/* initialize sockets (keep order here) */
	server_init();
	client_init();

	while (!sock_srv)
		l4thread_sleep(1);
	Kdebug("<>");
	i = 1;
	for (;;) {
		/* server writes */
		memcpy(buf_srv, data_srv, BUFLEN);
		if ((err = socket_write(sock_srv, buf_srv, BUFLEN)) < 0)
			ERR_EXIT("write(%d): %d", sock_srv, err);

		/* client reads */
		memset(buf_clt, 'X', BUFLEN);
		if ((err = socket_read(sock_clt, buf_clt, BUFLEN)) < 0)
			ERR_EXIT("read(%d): %d", sock_clt, err);
		else if (err == 0)
			ERR_EXIT("read returns 0");
		LOG_printf("[%04d] client: %.*s\n", i, BUFLEN, buf_clt);

		/* client writes */
		memcpy(buf_clt, data_clt, BUFLEN);
		if ((err = socket_write(sock_clt, buf_clt, BUFLEN)) < 0)
			ERR_EXIT("write(%d): %d", sock_clt, err);

		/* server reads */
		memset(buf_srv, 'X', BUFLEN);
		if ((err = socket_read(sock_srv, buf_srv, BUFLEN)) < 0)
			ERR_EXIT("read(%d): %d", sock_srv, err);
		else if (err == 0)
			ERR_EXIT("nothing read?");
		LOG_printf("[%04d] server: %.*s\n\n", i, BUFLEN, buf_srv);

		i++;                    //Kdebug("><");
	}
}

