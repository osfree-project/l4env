#include "utils.h"

int close_shutdown_test()
{
     int ret;
     int sockfd;
     int my_error=0;
     struct sockaddr_in socket_addr;
     static char string[2048];
     

     printf ("\n\nclose_shutdown_test()\n");
     printf ("close working.........................................");
     //socket, bind, listen
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "read_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  ret = close(sockfd);
	  if ( ret < 0){
	       my_error = 1;
	       my_error_out(error_datei, "close_shutdown_test: close error", 
			    errno, 0);
	       
	  }
     }
     my_output(&my_error);
     

     printf ("shutdown working......................................");
     //socket, bind, listen
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "read_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &socket_addr, sizeof( socket_addr ) );
	  socket_addr.sin_family = AF_INET;
	  socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	  socket_addr.sin_port = htons( TEST_PORT1 ); 
	  if (bind(sockfd, (struct sockaddr *) &socket_addr, 
		   sizeof(socket_addr)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "read_test: Error bind(sockfd, "\
			    "<sockaddr>, sizeof(<sockaddr>))", errno, 0);
	  }else
	       if (listen(sockfd, 5) < 0){
		    my_error = 1;
		    sprintf(string, "read_test: listen(%i, 5)", sockfd);
		    my_error_out(error_datei, string, errno, 0);
	       }
     	  ret = shutdown(sockfd, 2);
	  if ( ret < 0){
	       my_error = 1;
	       my_error_out(error_datei, "close_shutdown_test: shutdown error", 
			    errno, 0);
	  }
     }
     close(sockfd);
     my_output(&my_error);


     /*
      * 9
      */
     printf ("Error: Bad file descriptor............................");
     //socket, bind, listen
     close(1867576476);
     if ( errno != 9 ){
	  my_error = 1;
	  my_error_out(error_datei, "close_shutdown_test: close error", 
		       errno, 9);
     }
     my_output(&my_error);
     
     /*
      * 88
      */
     printf ("Error: Socket operation on non-socket.................");
     shutdown(2,2);
     if ( errno != 88 ){
	  my_error = 1;
	  my_error_out(error_datei, "close_shutdown_test: shutdown error", 
		       errno, 88);
     }
     my_output(&my_error);


     /*
      * 107
      */
     printf ("Error: Transport endpoint is not connected............");
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "read_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  shutdown(sockfd, 2);
	  if ( errno != 107){
	       my_error = 1;
	       my_error_out(error_datei, "close_shutdown_test: shutdown "\
			    "error", errno, 107);
	  }
     }
     my_output(&my_error);
     return 0;
}
