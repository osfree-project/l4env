
#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 

#define PORT 80    /* the port client will be connecting to */

#define MAXDATASIZE 1*1024 /* max number of bytes we can get at once */

char *msgbuf = "GET / HTTP/1.0\n\n";

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct sockaddr_in their_addr; /* connector's address information */

	printf("create socket\n");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("could not open socket\n");
		exit(1);
	}

	their_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//	inet_aton("192.168.76.1", &their_addr.sin_addr);
	their_addr.sin_family = AF_INET;      /* host byte order */
	their_addr.sin_port = htons(PORT);    /* short, network byte order */
	memset(&(their_addr.sin_zero), 0, 8);     /* zero the rest of the struct */

	printf("connect\n");
	if (connect(sockfd, (struct sockaddr *)&their_addr, \
				sizeof(struct sockaddr)) == -1) {
		printf("Error: connect\n");
		exit(1);
	}

	printf("send request\n");
	send(sockfd, msgbuf, strlen(msgbuf), 0);

	printf("receive\n");
	if ((numbytes=recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
		printf("Error: recv\n");
		exit(1);
	}

	buf[numbytes] = '\0';

	printf("Received %d bytes: %s",(int)numbytes,buf);

	printf("close socket\n");
	close(sockfd);

	return 0;
}
