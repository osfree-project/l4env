/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __GLIBC__

/* Linux-specific stuff */

#else

/* L4-specific stuff */
#include <l4/log/l4log.h>
#define printf LOG_printf

char LOG_tag[9] = "server";
unsigned l4libc_heapsize = 32*1024;

#endif

#define BUFSIZE 512

static char rcv_buf[BUFSIZE];
static char snd_buf[BUFSIZE];

static void error(char *msg)
{
	printf("[SERVER] ERROR %s (%d)\n", msg, errno);
	exit(1);
}

int main(int argc, char *argv[])
{
	int err;
	int sockfd, newsockfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;

	if (argc != 2) {
		printf("usage: prog <port>\n");
		exit(1);
	}

	printf("[SERVER] opening socket\n");
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) error("opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portno);
	err = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (err < 0) error("on binding");

	listen(sockfd, 1);
//	listen(sockfd, 5);
	while(1) {
		int i, n;

		printf("[SERVER] waiting for client\n");
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) error("on accept");
		printf("[SERVER] connected to client\n");

		printf("[SERVER] receiving\n");
		bzero(rcv_buf, BUFSIZE);
		n = read(newsockfd, rcv_buf, BUFSIZE);
		if (n < 0) error("reading from socket");

		printf("[SERVER] received message follows\n");
		for (i = 0; i < n; i++) printf("%c", rcv_buf[i]);
		printf("\n");

		printf("[SERVER] sending\n");
		for (i = 0; i < BUFSIZE; i++) snd_buf[i] = (i % 10) + 48;
		n = write(newsockfd, snd_buf, BUFSIZE);
		if (n < 0) error("writing to socket");

		printf("[SERVER] closing connection\n");
		close(newsockfd);
	}
	return 0; 
}
