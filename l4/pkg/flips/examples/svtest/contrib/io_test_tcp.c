#include "utils.h"

void test_sc_1();
int test_server_1(int my_test_port, int my_package_size);
int test_client_1(int my_test_port, const char *my_ip_addr, 
		  int my_package_size, const char *my_message);
void *thread_function_1(void *arg);


typedef struct tcp_send{
     int my_test_port;
     const char *my_ip_addr;
     int my_package_size;
     const char *my_message;
}tcp_s;


int io_test_tcp(int my_server_client, int my_test_port, int my_package_size,
		const char *my_ip_addr, const char *my_message)
{
     switch (my_server_client){
     case 0:
	  test_sc_1(my_test_port, my_package_size, my_ip_addr, my_message);
	  break;
     case 1:
	  test_server_1(my_test_port, my_package_size);
	  break;
     case 2:
	  test_client_1(my_test_port, my_ip_addr, my_package_size,
			my_message);
	  break;
     default:
	  break;
     }
     return 0;
}


void test_sc_1(int my_test_port, int my_package_size, const char *my_ip_addr,
	       const char *my_message)
{

int newsockfd;                     /* the fd for the client's socket */
     unsigned client_length;
     int ret, res;
     int my_sockfd;
     struct sockaddr_in my_socket_addr_recv;
     struct tcp_send send_struct;
     pthread_t my_thread;
     char buffer[my_package_size-1];
     
     send_struct.my_test_port = my_test_port;
     send_struct.my_ip_addr = my_ip_addr;
     send_struct.my_package_size = my_package_size;
     send_struct.my_message = my_message;

     if ((my_sockfd = my_socket(MY_TYPE_TCP))){
	  bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_recv.sin_port = htons( my_test_port );   
	  if (my_bind_server(my_sockfd, my_test_port, my_socket_addr_recv)){
	       
	       if (my_listen(my_sockfd)){
		    printf("server waiting on port: %i\n", my_test_port);
		    fflush(stdout);
		    //thread
		    res = pthread_create(&my_thread, NULL, thread_function_1, 
					 &send_struct);
		    if (res != 0){
			 my_error_out(error_datei, "io_test_tcp: Thread "\
				      "creation failed" , errno, 0);
		    }
		    client_length = sizeof(my_socket_addr_recv);
		    newsockfd = accept(my_sockfd, (struct sockaddr *) 
				       &my_socket_addr_recv, &client_length);
		    if(newsockfd < 0){
			 my_error_out(error_datei, "io_test_tcp: Error "\
				      "accept()", errno, 0);
		    }else{
			 ret = read_from_client_buf(newsockfd, 
						    my_package_size-1, buffer);
			 if (ret < 0){
			      my_error_out(error_datei, "io_test_tcp: Size "\
					   "Error on read", 1, 0);
			 }
			 buffer[ret] = '\0';
			 printf("message: received:%s\n",buffer);
			 
		    }
		    close(my_sockfd);
		    close(newsockfd);
	       }
	  }
     
     }
}


//thread
//test_client_1(my_test_port, my_ip_addr, my_package_size,
//my_message);

void *thread_function_1(void *arg)
{

     int my_sockfd;
     int ret;
     struct sockaddr_in my_socket_addr_recv;
     struct sockaddr_in my_socket_addr_send;
     struct tcp_send *arguments;

     arguments = arg;
     

     if ((my_sockfd = my_socket(MY_TYPE_TCP))){
	 
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = inet_addr(arguments->my_ip_addr);
	  my_socket_addr_recv.sin_port = htons( arguments->my_test_port ); 
	  my_socket_addr_send.sin_family = MY_FAMILY;
	  my_socket_addr_send.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_send.sin_port = htons( 0 ); 	
	 
	  if( (ret = connect(my_sockfd, (struct sockaddr *) 
			     &my_socket_addr_recv, 
			     sizeof(my_socket_addr_recv))) < 0) {
	       my_error_out(error_datei, "io_test_tcp: Error connect()", 
			    errno, 0);
	  }else{
	       ret = write_to_server(my_sockfd, strlen(arguments->my_message), 
				     arguments->my_message);
	       if (ret < 0){
		    my_error_out(error_datei, 
				 "io_test_tcp: error occurrence by "\
				 "write(int fd, const char *buf, "\
				 "size_t count)", 1, 0);
	       }else{
		    printf("client: %i bytes writen\n",ret);
		    
	       }
	       
	       close(my_sockfd);
	  }
     }


     pthread_exit(&ret);   

	 /* not reached */
	 return NULL;
}







//
int test_server_1(int my_test_port, int my_package_size)
{

     int newsockfd;                     /* the fd for the client's socket */
     unsigned client_length;
     int ret;
     int my_sockfd;
     struct sockaddr_in my_socket_addr_recv;
     char buffer[my_package_size-1];
     
     

     if ((my_sockfd = my_socket(MY_TYPE_TCP))){
	  bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_recv.sin_port = htons( my_test_port );   
	  if (my_bind_server(my_sockfd, my_test_port, my_socket_addr_recv)){
	       
	       if (my_listen(my_sockfd)){
		    printf("server waiting on port: %i\n", my_test_port);
		    fflush(stdout);
			 
		    client_length = sizeof(my_socket_addr_recv);
		    newsockfd = accept(my_sockfd, (struct sockaddr *) 
				       &my_socket_addr_recv, &client_length);
		    if(newsockfd < 0){
			 my_error_out(error_datei, "io_test_tcp: Error "\
				      "accept()", errno, 0);
		    }else{
			 ret = read_from_client_buf(newsockfd, 
						    my_package_size-1, buffer);
			 if (ret < 0){
			      my_error_out(error_datei, "io_test_tcp: Size "\
					   "Error on read", 1, 0);
			 }
			 buffer[ret] = '\0';
			 printf("message: received:%s\n",buffer);
			 
		    }
		    close(my_sockfd);
		    close(newsockfd);
	       }
	  }
     
     }


     return 1;
}

//
int test_client_1(int my_test_port, const char *my_ip_addr, int my_package_size,
		  const char *my_message)
{
     int my_sockfd;
     int ret;
     struct sockaddr_in my_socket_addr_recv;
     struct sockaddr_in my_socket_addr_send;


     if ((my_sockfd = my_socket(MY_TYPE_TCP))){
	 
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = inet_addr(my_ip_addr);
	  my_socket_addr_recv.sin_port = htons( my_test_port ); 
	  my_socket_addr_send.sin_family = MY_FAMILY;
	  my_socket_addr_send.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_send.sin_port = htons( 0 ); 	
	 
	  if( (ret = connect(my_sockfd, (struct sockaddr *) 
			     &my_socket_addr_recv, 
			     sizeof(my_socket_addr_recv))) < 0) {
	       my_error_out(error_datei, "io_test_tcp: Error connect()", 
			    errno, 0);
	  }else{
	       ret = write_to_server(my_sockfd, strlen(my_message), my_message);
	       if (ret < 0){
		    my_error_out(error_datei, 
				 "io_test_tcp: error occurrence by "\
				 "write(int fd, const char *buf, size_t "\
				 "count)", 1, 0);
	       }else{
		    printf("client: %i bytes writen\n",ret);
		    
	       }
	       close(my_sockfd);
	  }
     }
     return 1;
     
}
