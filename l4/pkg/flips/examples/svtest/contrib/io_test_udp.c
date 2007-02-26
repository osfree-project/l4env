#include "utils.h"
#include <stdio.h>

void test_sc(int my_package_size, int my_test_port, int my_package_count, 
	     const char *my_ip_addr,const char *my_message);
int test_server(int my_package_size, int my_test_port, 
		int my_package_count);
int test_client(int my_package_size, int my_test_port, 
		int my_package_count, const char *my_message,
		const char *my_ip_addr);
void *thread_function(void *arg);

typedef struct thread_send{
	  const char *ip_addr;
	  int test_port;
	  int package_count;
	  const char *message;
} ts;



/*
 * 0 Server + Client
 * 1 Server
 * 2 Client
 */
int io_test_udp(int my_package_size,int my_test_port, 
		int my_package_count, const char *my_ip_addr,
		int my_server_client, const char *my_message)
{
     switch (my_server_client){
     case 0:
	  test_sc(my_package_size, my_test_port, my_package_count, my_ip_addr,
		  my_message);
	  break;
     case 1:
	  test_server(my_package_size, my_test_port, my_package_count);
	  break;
     case 2:
	  test_client(my_package_size, my_test_port, 
		      my_package_count, my_message, my_ip_addr);
	  break;
     default:
	  break;
     }
     return 0;
}

void test_sc(int my_package_size, int my_test_port, 
	     int my_package_count, const char *my_ip_addr,
	     const char *my_message)
{
     int res;
     pthread_t my_thread;
     int sockfd_server;
     unsigned client_addr_length;
     int n_bytes_received;
     int count_messages = 0;
     char my_recv_buf[my_package_size];    /* buffer to store received data */
     struct sockaddr_in my_socket_addr_recv;
     struct sockaddr_in my_socket_addr_send;
     

     struct thread_send th_s;
     
     th_s.ip_addr = my_ip_addr;
     th_s.test_port = my_test_port;
     th_s.package_count = my_package_count;
     th_s.message = my_message;
     
     
     

     if( (sockfd_server = socket(MY_FAMILY, MY_TYPE_UDP, 0)) < 0){
	  my_error_out(error_datei, "io_test_udp: Error socket(MY_FAMILY, "\
		       "MY_TYPE_UDP, 0)", errno, 0);
     }else{
	  bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_recv.sin_port = htons( my_test_port ); 
	  if (bind(sockfd_server, (struct sockaddr *) &my_socket_addr_recv, 
		   sizeof(my_socket_addr_recv)) < 0){
	       my_error_out(error_datei, "io_test_udp: Error bind(sockfd, "\
			    "<sockaddr>, sizeof(<sockaddr>)) server", 
			    errno, 0);
	  }else{
	       res = pthread_create(&my_thread, NULL, thread_function, &th_s);
	       if (res != 0){
		    my_error_out(error_datei, "io_test_ucp: Thread creation "\
				 "failed" , errno, 0);
	       }
	       //
	         while(count_messages < my_package_count){
		   if (debug)
		     printf("server wartet...%i %i\n",my_package_size, 
			    my_test_port);
		    client_addr_length = sizeof(struct sockaddr);     
       		    n_bytes_received = recvfrom(sockfd_server, my_recv_buf, 
						my_package_size, 0,
						(struct sockaddr*) 
						&my_socket_addr_send, 
						&client_addr_length);
		    if (n_bytes_received == -1){
			 my_error_out(error_datei, "io_test_udp: Error by "\
				      "receiving data", errno, 0);  
			 //return 0;
		    }
		    my_recv_buf[n_bytes_received]='\0';      

		    printf("\n%3d. Got packet from %s:%i %ibytes\n", 
			   count_messages, 
			   inet_ntoa(my_socket_addr_send.sin_addr), 
			   my_socket_addr_recv.sin_port, 
			   n_bytes_received);
		    if (debug)
		      printf("io_test_udp: test_sc: %s\n",my_recv_buf);

		    count_messages++;
 	       }
	  }
     }
}
//thread
//void *thread_function(const char *my_ip_addr, int my_test_port, 
//		      int my_package_count, const char *my_message)
void *thread_function(void *arg)
{
     int ret = 0;
     int sockfd_client;
     int n_bytes_send;
     int count_messages = 0;     
     struct sockaddr_in recv_addr;
     struct sockaddr_in my_socket_addr_send;
     struct thread_send *arguments;

     arguments = arg;
     
     sleep(2);//waiting for server
     bzero((char *) &recv_addr, sizeof( recv_addr ) );
     recv_addr.sin_family = MY_FAMILY;
     recv_addr.sin_addr.s_addr = inet_addr( arguments->ip_addr);
     recv_addr.sin_port = htons ( arguments->test_port);
     if (debug)
       printf("io_test_udp: thread_function: my_test_port:%i\n",
	      arguments->test_port);
     
     if( (sockfd_client = socket(MY_FAMILY, MY_TYPE_UDP, 0)) < 0){
	  my_error_out(error_datei, "io_test_udp: Error socket(MY_FAMILY, "\
		       "MY_TYPE_UDP, 0)", errno, 0);
     }else{
	  bzero((char *) &my_socket_addr_send, sizeof( my_socket_addr_send ) );
	  my_socket_addr_send.sin_family = MY_FAMILY;
	  my_socket_addr_send.sin_addr.s_addr = htonl(INADDR_ANY);
	  my_socket_addr_send.sin_port = htons( 0 ); 
	  if (bind(sockfd_client, (struct sockaddr *) &my_socket_addr_send, 
		   sizeof(my_socket_addr_send)) < 0){
	       ret = 1;
	       my_error_out(error_datei, 
			    "io_test_udp: Error bind(sockfd, <sockaddr>, "\
			    "sizeof(<sockaddr>)) client", 
			    errno, 0);
	  }else{
	       //
	        while(count_messages < arguments->package_count){
		     n_bytes_send = sendto(sockfd_client, arguments->message, 
					  strlen(arguments->message), 0,
					  (struct sockaddr *)&recv_addr, 
					  sizeof(struct sockaddr));
		    if (n_bytes_send == -1){
			 ret = 1;
			 my_error_out(error_datei, "io_test_udp: Error by "\
				      "sending data", errno, 0);  
		    }
		    printf("\tsent %d bytes to %s:%i\n", n_bytes_send, 
			   inet_ntoa(recv_addr.sin_addr), 
			   recv_addr.sin_port);
		    count_messages++;
	       } 
	  }
	  
     }
     pthread_exit(&ret);   

	 /* not reached */
	 return NULL;
}


//receiver
int test_server(int my_package_size, int my_test_port, 
		int my_package_count)
{
     unsigned client_addr_length;
     int n_bytes_received;
     int count_messages = 0;
     char my_recv_buf[my_package_size];    /* buffer to store received data */
     int my_sockfd;
     struct sockaddr_in my_socket_addr_recv;
     struct sockaddr_in my_socket_addr_send;

     if (debug)
       printf ("server hier on port:%i...\n",my_test_port);
     if ( (my_sockfd = my_socket(MY_TYPE_UDP)) ){
	  /*
	  bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = htonl( INADDR_ANY );
	  my_socket_addr_recv.sin_port = htons( my_test_port ); 
	  */
	  if (my_bind_server(my_sockfd, my_test_port, my_socket_addr_recv)){
	       while(count_messages < my_package_count){
		    client_addr_length = sizeof(struct sockaddr);     
       		    n_bytes_received = recvfrom(my_sockfd, my_recv_buf, 
						my_package_size, 0,
						(struct sockaddr*) 
						&my_socket_addr_send, 
						&client_addr_length);
		    if (n_bytes_received == -1){
			 my_error_out(error_datei, "io_test_udp: Error by "\
				      "receiving data", errno, 0);  
			 return 0;
		    }
		    my_recv_buf[n_bytes_received]='\0';      

		    printf("\n%3d. Got packet from %s:%i %ibytes\n", 
			   count_messages, 
			   inet_ntoa(my_socket_addr_send.sin_addr), 
			   my_socket_addr_recv.sin_port, 
			   n_bytes_received);
		    if (debug)
		      printf("%s\n",my_recv_buf);

		    count_messages++;
 	       }
	  }
     }
     
     close(my_sockfd);
     
     return 1;
     
     
}

//sender
int test_client(int my_package_size, int my_test_port, 
		int my_package_count, const char *my_message,
		const char *my_ip_addr)
{
     int my_sockfd;
     int n_bytes_send;
     int count_messages = 0;
     struct sockaddr_in my_socket_addr_recv;
     struct sockaddr_in my_socket_addr_send;

     if ((my_sockfd = my_socket(MY_TYPE_UDP))){
	  bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
	  my_socket_addr_recv.sin_family = MY_FAMILY;
	  my_socket_addr_recv.sin_addr.s_addr = inet_addr( my_ip_addr);
	  my_socket_addr_recv.sin_port = htons ( my_test_port);	  
	  
	  if (my_bind_client(my_sockfd, my_test_port, &my_socket_addr_recv, 
			     &my_socket_addr_send, my_ip_addr)){
	  
	       while(count_messages < my_package_count){
		 if (debug)
		   printf("udp client sendet...%i %i\n",my_package_size, 
			  my_test_port);
		 n_bytes_send = sendto(my_sockfd, my_message, 
				       strlen(my_message), 0,
				       (struct sockaddr *)
				       &my_socket_addr_recv, 
				       sizeof(struct sockaddr));
		    if (n_bytes_send == -1){
			 my_error_out(error_datei, "io_test_udp: Error by "\
				      "sending data", errno, 0);  
			 return 0;
		    }
		    printf("\tsent %d bytes to %s:%i\n", n_bytes_send, 
			   inet_ntoa(my_socket_addr_recv.sin_addr), 
			   my_socket_addr_recv.sin_port);
		    count_messages++;
	       }
	       close(my_sockfd);
	  }
     }
     
     return 1;
}

		  
