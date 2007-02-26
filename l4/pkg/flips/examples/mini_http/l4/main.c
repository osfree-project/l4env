/*
 * \brief   Mini ifconfig test client
 * \date    2003-08-07
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/

l4_ssize_t l4libc_heapsize = 500*1024;
l4_threadid_t srv;
char LOG_tag[9] = "httpserv";

#define _DEBUG 0
#define _INFO  0
#define BUFSIZE 4096
#define MAX_TIMEOUT 60
static char recvbuf[BUFSIZE];
static char sendbuf[BUFSIZE];

char *httpmsg = 
	"HTTP/1.1 200 OK\n"
	"Date: Mon, 04 Aug 2003 13:01:26 GMT\n"
	"Server: mini_http/cvs (L4Env) DROPS\n"
	"Connection: close\n"
	"Content-Type: text/html; charset=iso-8859-1\n"
	"\n"
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
	"<HTML><HEAD>\n"
	"<TITLE>FLIPS Mini HTTP server</TITLE>\n"
	"</HEAD><BODY bgcolor=#778899>\n"
	"<H1>Welcome to the HTTP server running on FLIPS</H1>\n"
	"Thanks for visiting our informative site.\n"
	"You are visitor number %d.<P>\n"
	"<P>some garbage follows..."
	"<P>...computer hardware progress is so fast.  No other technology since civilization began has seen six orders of magnitude in performance-price gain in 30 years.  -- Fred Brooks, Jr."
	"<P>To be excellent when engaged in administration is to be like the North Star.  As it remains in its one position, all the other stars surround it.  -- Confucius"
	"<P>Marriage is the triumph of imagination over intelligence.  Second marriage is the triumph of hope over experience.  "
	"<P>algorithm, n.: Trendy dance for hip programmers.  "
	"<P>An American is a man with two arms and four wheels.  -- A Chinese child "
	"<P>In Lexington, Kentucky, it's illegal to carry an ice cream cone in your pocket.  "
	"</BODY></HTML>\n";

int main(int argc, char **argv) {
	int s, c, err;
	static int errcount, visitor;
	struct sockaddr_in addr;
	fd_set rfds;
	struct timeval tv;

	if ((argc < 2) || strcmp("--nowait", argv[1]))
		while (names_waitfor_name("ifconfig", &srv, 10000) == 0) {
			LOG("Client(main): \"mini_ifconfig\" not available, keep trying...");
		}

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	LOGd(_DEBUG,"try to open socket");
	if ((s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		LOG("Couldn't create socket");
		return -1;
	}
	if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
		LOG("Couldn't bind");
		return -1;
	}
	LOGd(_DEBUG,"bound to socket %d, call listen",s);
	if ((err=listen(s, 1)) < 0) {
		LOG("listen(%d): %d", s, err);
		return -1;
	}

	LOGd(_INFO,"HTTP server is up and accepting...");

	names_register("http");

	for (;;) {
		unsigned int addrlen = sizeof(struct sockaddr);
		struct sockaddr dummyaddr;
		if ((c=accept(s, &dummyaddr, &addrlen)) < 0) {
			LOG("accept(%d): %d", s, c);
			if (++errcount > 99) {
				LOG("maximum error count reached---giving up...");
				exit(1);
			}
			continue;
		}
		

		LOGd(_DEBUG,"accept passed - go into select.");
		
		/* Watch fd to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(c, &rfds);

		tv.tv_sec = MAX_TIMEOUT;
		tv.tv_usec = 0;

		err = select(c+1, &rfds, NULL, NULL, &tv);

		LOGd(_DEBUG,"select passed");

		if (err != 1) {
			LOGd(_DEBUG, "timeout, close connection");
			goto close_connection;
		}

		err = recv(c, recvbuf, BUFSIZE-1, 0);
		LOGd(_DEBUG,"err %d, recbuf = %s", err, recvbuf);

		snprintf(sendbuf, BUFSIZE, httpmsg, ++visitor);
		LOGd(_INFO,"visitor: %d", visitor);

		send(c, sendbuf, strlen(sendbuf), 0);
close_connection:
		shutdown(c, 1);
		close(c);
	}
	return 0;
}

