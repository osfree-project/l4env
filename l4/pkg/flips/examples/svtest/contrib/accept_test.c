#include "utils.h"

void *thread_client(void *);

/*
 * test the command accept
 * int accept(int s, struct sockaddr *addr, int *addrlen);
 */
int accept_test(){
     int sockfd;
     unsigned client_length;
     int my_error=0;
     int tmp;
     char string[2048];
     struct sockaddr_in socket_addr;
     pthread_t thread_1;
     int ret_thread;
     void *thread_result;
     
     printf ("\n\naccept_test()\n");
     printf("accept working........................................");
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "accept_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &socket_addr, sizeof( socket_addr ) );
	  socket_addr.sin_family = AF_INET;
	  socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	  socket_addr.sin_port = htons( TEST_PORT1 ); 
	  if (bind(sockfd, (struct sockaddr *) &socket_addr, 
		   sizeof(socket_addr)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "accept_test: Error "\
			    "bind(sockfd, <sockaddr>, sizeof(<sockaddr>))", 
			    errno, 0);
	  }else
	       if (listen(sockfd, 5) < 0){
		    my_error = 1;
		    sprintf(string, "accept_test: listen(%i, 5)", sockfd);
		    my_error_out(error_datei, string, errno, 0);
	       }
     }
     fflush(stdout);
     fflush(error_datei);
     if(my_error == 0){
	  int newsockfd;

	  ret_thread = pthread_create(&thread_1, NULL, thread_client, NULL); 
	  if (ret_thread != 0){
	       my_error = 1;
	       my_error_out(error_datei, "accept_test: Thread creation "\
			    "failed" , errno, 0);
	  }
	  //Server
	  client_length = sizeof(socket_addr);
	  newsockfd = accept(sockfd, (struct sockaddr *) &socket_addr, 
			     &client_length);
	  if(newsockfd < 0){
	       my_error = 1;
	       my_error_out(error_datei, "accept_test: Error accept()", 
			    errno, 0);
	  }
          close(newsockfd);
     }
     fflush(stdout);
     ret_thread = pthread_join(thread_1, &thread_result);
     my_output(&my_error);
     
     

     /*
      * Socket operation on non-socket 88
      */
     printf ("Error: Socket operation on non-socket.................");
     for(tmp=0; tmp < 3; tmp++){
	  client_length = sizeof(socket_addr);
	  if(accept(tmp, (struct sockaddr *) &socket_addr, &client_length) < 0){
	       if( errno != 88){
		    my_error = 1;
		    my_error_out(error_datei, "accept_test: accept()", 
				 errno, 88);
	       }
	  }else{
	       my_error = 1;
	       my_error_out(error_datei, "accept_test: accept()", 0, 9);
	  }
     }
     my_output(&my_error);
     close(sockfd);
     return 0;
}



/* 
 * Client as thread
 */
void *thread_client(void *p)
{
     int c_sockfd, ret;
     int my_error=0;
     struct sockaddr_in c_socket_addr;
     
     if( (c_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "accept_test: Error client "\
		       "socket(AF_INET, SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &c_socket_addr, sizeof( c_socket_addr ) );
	  c_socket_addr.sin_family = AF_INET;
	  c_socket_addr.sin_addr.s_addr = inet_addr( LOCALHOST );
	  c_socket_addr.sin_port = htons( TEST_PORT1 ); 
	  //sleep(5);
	  if( (ret = connect(c_sockfd, (struct sockaddr *) &c_socket_addr, 
			     sizeof(c_socket_addr))) < 0) {
	       my_error = 1;
	       my_error_out(error_datei, "accept_test: Error connect()", 
			    errno, 0);
	  }
     }
     close(c_sockfd);
     pthread_exit(&my_error);

	 /* not reached */
	 return NULL;
}
