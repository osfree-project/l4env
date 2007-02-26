#include "utils.h"


/*
 * test the bind command
 * int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
 */
int bind_test(){
	int sockfd=0;
	int i=0;
	int tmp;
	int my_error=0;
	char string[2048];
	struct sockaddr_in socket_addr;

	/*
	 * Bind working OK
	 */
	printf ("\n\nbind_test()\n");
	printf ("Bind working..........................................");
	bzero( &socket_addr, sizeof( socket_addr ) );
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	socket_addr.sin_port = htons( TEST_PORT ); 
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) > 0){
		if( bind(sockfd , (struct sockaddr *) &socket_addr, 
			 sizeof(socket_addr) ) < 0){
			my_error = 1;
			sprintf(string,"bind_test: bind(%i, <sockaddr>, "\
				"sizeof(<sockaddr>))",sockfd);
			my_error_out(error_datei, string, errno, 0);
		}
	}else{
		my_error = 1;
		my_error_out(error_datei,"bind_test: Error on "\
			     "socket(AF_INET, SOCK_STREAM, 0)", errno, 0);
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung:  Bad file descriptor 9
	 */
	printf ("Error: Bad file descriptor............................");
	for(i=10000; i < 20000; i++)
		if( bind(i , NULL, (int)NULL) < 0){
			if( errno != 9){
				my_error = 1;
				sprintf(string, "bind_test: "\
					"bind(%i, NULL, NULL)",i);
				my_error_out(error_datei, string, errno, 9);
			}
		}else{
			my_error = 1;
			sprintf(string, "bind_test: bind(%i, NULL, NULL)",i);
			my_error_out(error_datei, string, 0, 9);
		}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Socket operation on non-socket 88
	 */
	printf ("Error: Socket operation on non-socket.................");
	for(i=0; i < 3; i++){
		if( bind(i , NULL, (int)NULL) < 0){
			if( errno != 88){
				my_error = 1;
				sprintf(string, "bind_test: "\
					"bind(%i, NULL, NULL)",i);
				my_error_out(error_datei, string, errno, 88);
			}
		}else{
			my_error = 1;
			sprintf(string, "bind_test: bind(%i, NULL, NULL)",i);
			my_error_out(error_datei, string, 0, 88);
		}
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Permission denied 13
	 */
	printf ("Error: Permission denied..............................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "bind_test: Error "\
			     "socket(AF_INET, SOCK_STREAM, 0)", errno, 0);
	}
	else{
		bzero( &socket_addr, sizeof( socket_addr ) );
		socket_addr.sin_family = AF_INET;
		socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		for(tmp=1; tmp < 1024; tmp++){
			socket_addr.sin_port = htons( tmp );
			if (bind(sockfd, (struct sockaddr *) &socket_addr, 
				 sizeof(socket_addr)) < 0){
				if( errno != 13){
					my_error = 1;
					my_error_out(error_datei, "bind_test: "\
						     "bind(%i, <sockaddr>, "\
						     "sizeof(sockaddr))", 
						     errno, 13);
				}
			}
			else{
				my_error = 1;
				my_error_out(error_datei, "bind_test: "\
					     "bind(%i, <sockaddr>, "\
					     "sizeof(sockaddr))", 0, 13);
			}
		}
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Bad address 14
	 */
	printf ("Error: Bad address....................................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "bind_test: Error socket(AF_INET, "\
			     "SOCK_STREAM, 0)", errno, 0);
	}else{
		if (bind(sockfd, NULL, sizeof(socket_addr)) < 0){
			if( errno != 14){
				my_error = 1;
				sprintf(string, "bind_test: bind(%i, NULL, "\
					"sizeof(socket_addr))",sockfd);
				my_error_out(error_datei, string, errno, 14);
			}
		}else{
			my_error = 1;
			sprintf(string, "bind_test: bind(%i, NULL, "\
				"sizeof(socket_addr))",sockfd);
			my_error_out(error_datei, string, 0, 14);
		}
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Invalid argument 22 
	 */
	printf ("Error: Invalid argument...............................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "bind_test: Error socket(AF_INET, "\
			     "SOCK_STREAM, 0)", errno, 0);
	}else{
		for(tmp=2345; tmp < 15000; tmp++){
			if (bind(sockfd, NULL, tmp) < 0){
				if( errno != 22){
					my_error = 1;
					sprintf(string, "bind_test: "\
						"bind(%i, NULL, %i)", 
						sockfd, tmp);
					my_error_out(error_datei, string, 
						     errno, 22);
				}
			}else{
				my_error = 1;
				sprintf(string, "bind_test: bind(%i, NULL, %i)", 
					sockfd, tmp);
				my_error_out(error_datei, string, 0, 22);
			}
		}
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Address already in use 98
	 */
	printf ("Error: Address already in use.........................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "bind_test: Error socket(AF_INET, "\
			     "SOCK_STREAM, 0)", errno, 0);
	}else{
		bzero((char *) &socket_addr, sizeof( socket_addr ) );
		socket_addr.sin_family = AF_INET;
		socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		socket_addr.sin_port = htons( TEST_PORT );
		if (bind(sockfd, (struct sockaddr *) &socket_addr, 
			 sizeof(socket_addr)) < 0){
			if( errno != 98){
				my_error = 1;
				sprintf(string, "bind_test: bind(%i, "\
					"<sockaddr>, sizeof(<sockaddr>))", 
					sockfd);
				my_error_out(error_datei, string, errno, 98);
			}
		}
		else{
			my_error = 1;
			sprintf(string, "bind_test: bind(%i, <sockaddr>, "\
				"sizeof(<sockaddr>))",sockfd);
			my_error_out(error_datei, string, 0, 98);
		}
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Cannot assign requested address 99
	 */
	printf ("Error: Cannot assign requested address................");
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		my_error = 1;
		my_error_out(error_datei, "bind_test: Error socket(AF_INET, "\
			     "SOCK_STREAM, 0)", errno, 0);
	}else{
		for (tmp=130; tmp < 210; tmp++){
			bzero( &socket_addr, sizeof( socket_addr ) );
			socket_addr.sin_family = AF_INET;
			socket_addr.sin_addr.s_addr = tmp;
			socket_addr.sin_port = htons( TEST_PORT ); 
			if (bind(sockfd, (struct sockaddr *) &socket_addr, 
				 sizeof(socket_addr)) < 0){
				if( errno != 99){
					my_error = 1;
					sprintf(string, "bind_test: "\
						"bind(%i, <sockaddr>, "\
						"sizeof(<sockaddr>))",sockfd);
					my_error_out(error_datei, string, 
						     errno, 99);
				}
			}else{
				my_error = 1;
				sprintf(string, "bind_test: "\
					"bind(%i, <sockaddr>, "\
					"sizeof(<sockaddr>))",sockfd);
				my_error_out(error_datei, string, 0, 99);
			}
		}
	}
	my_output(&my_error);
	close(sockfd);
	return 0;
}

