/* A simple client in the internet domain using TCP
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
#include <arpa/inet.h>
#include <netdb.h> 

#ifdef __GLIBC__

/* Linux-specific stuff */

#else

/* L4-specific stuff */
#include <l4/log/l4log.h>
#define printf LOG_printf

char LOG_tag[9]="client";
unsigned l4libc_heapsize = 32*1024;

#endif

#define IP "127.0.0.1"

#define BUFSIZE 512

static char rcv_buf[BUFSIZE];
static char snd_buf[BUFSIZE];

static void error(char *msg)
{
	printf("[CLIENT] ERROR %s (%d)\n", msg, errno);
	exit(1);
}

int main(int argc, char *argv[])
{
	int n, i;
	int sockfd, portno, err;
	struct sockaddr_in serv_addr;

	if (argc != 2) {
		printf("usage: prog <port>\n");
		exit(1);
	}

	printf("[CLIENT] opening socket\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	inet_aton(IP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(portno);

	printf("[CLIENT] connecting\n");
	err = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (err < 0) {
		printf("[CLIENT] connect() returned %d\n", err);
		error("connecting");
	}

	printf("[CLIENT] sending\n");
	for (i = 0; i < BUFSIZE; i++) snd_buf[i] = i / 10 + 65;
	n = write(sockfd, snd_buf, BUFSIZE);
	if (n < 0) error("writing to socket");

	bzero(rcv_buf, BUFSIZE);
	n = read(sockfd, rcv_buf, BUFSIZE);
	if (n < 0) error("reading from socket");

	printf("[CLIENT] received message follows\n");
	for (i = 0; i < n; i++) printf("%c", rcv_buf[i]);
	printf("\n");

	return 0;
}
