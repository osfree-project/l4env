#include "utils.h"

/*
 * test the command listen
 * int listen(int s, int backlog);
 */
int listen_test ()
{
	int tmp;
	int sockfd;
	int my_error=0;
	char string[2048];
	struct sockaddr_in socket_addr;

		/*
	 * listen
	 */
	printf ("\n\nlisten_test()\n");
	printf ("listen working........................................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "listen_test: Error socket(AF_INET, SOCK_STREAM, 0)", errno, 0);
	}else{
		bzero((char *) &socket_addr, sizeof( socket_addr ) );
		socket_addr.sin_family = AF_INET;
		socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		socket_addr.sin_port = htons( TEST_PORT+1 ); //Port > 1024 aber belegt
		if (bind(sockfd, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) < 0){
			my_error = 1;
			my_error_out(error_datei, "listen_test: Error bind(sockfd, <sockaddr>, sizeof(<sockaddr>))", 
			 errno, 0);
		}else
			if (listen(sockfd, 5) < 0){
				my_error = 1;
				sprintf(string, "listen_test: listen(%i, 5)", sockfd);
				my_error_out(error_datei, string, errno, 0);
			}
	}
	close(sockfd);
	my_output(&my_error);

	/*
	 * Bad file descriptor 9
	 */
	printf ("Error: Bad file descriptor............................");
	fflush(stdout);
	for(tmp=-100; tmp < 0; tmp++){
		if(listen(tmp, 5) < 0){
			if( errno != 9){
				my_error = 1;
				sprintf(string, "listen_test: listen(%i, 5)", tmp);
				my_error_out(error_datei, string, errno, 9);
			}
		}else{
			my_error = 1;
			sprintf(string, "listen_test: listen(%i, 5)", sockfd);
			my_error_out(error_datei, string, 0, 9);
		}
	}
	my_output(&my_error);

	/*
	 * Socket operation on non-socket 88
	 */
	printf ("Error: Socket operation on non-socket.................");
	for(tmp=0; tmp < 3; tmp++){
		if(listen(tmp, 5) < 0){
			if( errno != 88){
				my_error = 1;
				sprintf(string, "listen_test: listen(%i, 5)", tmp);
				my_error_out(error_datei, string, errno, 88);
			}

		}else{
			my_error = 1;
			sprintf(string, "listen_test: listen(%i, 5)", sockfd);
			my_error_out(error_datei, string, 0, 9);
		}
	}
	my_output(&my_error);
	close(sockfd);
	return 0;
}


