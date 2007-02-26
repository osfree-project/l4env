#include "utils.h"

#define MESSAGE "ick bin all hier!"
#define MESSAGESIZE 18 //17+1 wegen \0
#define BUFFSIZE 100
#define MAGIC_NR 0x6f


void *thread_test_ok();
void *thread_buffer_check();
int read_from_client (int fd, int size);
int read_from_client_c (int fd);


int read_from_client_buffer (int fd, int size);


/*
 * test the read and write command
 * ssize_t read(int fd, void *buf, size_t count);
 * ssize_t write(int fd, const char *buf, size_t count);
 */
int io_test()
{
     int sockfd, newsockfd;
     unsigned client_length,i;
     int my_error=0;
     int ret;
     char string[2048];
     char string_1[MESSAGESIZE+1];
     struct sockaddr_in socket_addr;
     
     pthread_t thread_1;
     void *thread_result;
     int ret_thread;

     printf ("\n\nio_test()\n");
     printf ("read-write working....................................");
     //socket, bind, listen


     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &socket_addr, sizeof( socket_addr ) );
	  socket_addr.sin_family = AF_INET;
	  socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	  socket_addr.sin_port = htons( TEST_PORT1 ); 
	  if (bind(sockfd, (struct sockaddr *) &socket_addr, 
		   sizeof(socket_addr)) < 0){
	       my_error_out(error_datei, 
			    "io_test: Error bind(sockfd, <sockaddr>, "\
			    "sizeof(<sockaddr>))", 
			    errno, 0);
	  }else
	       if (listen(sockfd, 5) < 0){
		    my_error = 1;
		    sprintf(string, "io_test: listen(%i, 5)", sockfd);
		    my_error_out(error_datei, string, errno, 0);
	       }
     }

     fflush(stdout);
     fflush(error_datei);
     ret_thread = pthread_create(&thread_1, NULL, thread_test_ok, NULL);
     if (ret_thread != 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Thread creation failed" , 
		       errno, 0);
     }
     //server
     client_length = sizeof(socket_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &socket_addr, 
			&client_length);
     if(newsockfd < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error accept()", errno, 0);
     }else{
	  ret = read_from_client(newsockfd, MESSAGESIZE);
	  if (ret < 0){
	       my_error = 1;
	       my_error_out(error_datei, "io_test: Size Error on read", 1, 0);
	  }
	  
     }
     ret_thread = pthread_join(thread_1, &thread_result);
     my_output(&my_error);
     close(sockfd);
     close(newsockfd);
	       
     

     
     /*
      * Buffersize check
      */
     printf ("buffersize check......................................");
     //socket, bind, listen
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &socket_addr, sizeof( socket_addr ) );
	  socket_addr.sin_family = AF_INET;
	  socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	  socket_addr.sin_port = htons( TEST_PORT1 ); 
	  if (bind(sockfd, (struct sockaddr *) &socket_addr, 
		   sizeof(socket_addr)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "io_test: Error bind(sockfd, "\
			    "<sockaddr>, sizeof(<sockaddr>))", 
			    errno, 0);
	  }else
	       if (listen(sockfd, 5) < 0){
		    my_error = 1;
		    sprintf(string, "io_test: listen(%i, 5)", sockfd);
		    my_error_out(error_datei, string, errno, 0);
	       }
     }
     fflush(stdout);
     fflush(error_datei);
     ret_thread = pthread_create(&thread_1, NULL, thread_test_ok, NULL);
     if (ret_thread != 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Thread creation failed" , 
		       errno, 0);
     }
     //server
     client_length = sizeof(socket_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &socket_addr, 
			&client_length);
     if(newsockfd < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error accept()", errno, 0);
     }else{
	  ret = read_from_client_buffer(newsockfd, MESSAGESIZE-15);
	  switch (ret){
	  case 0:
	       break;
	  case 1:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer begin to zu zeitig", 
			    1, 0);
	       break;
	  case 2:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer ist ausserhalb "\
			    "seines bereich", 1, 0);
	       break;
	  case 3:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: zu zeitig und ausserhalb "\
			    "des bereich", 1, 0);
	       break;
	  case 4:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer überschreibt sein "\
			    "ende", 1, 0);
	       break;
	  case 5:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer überschreibt anfang "\
			    "und ende", 1, 0);
	       break;
	  case 6:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer ausserhalb seines "\
			    "breich und überschreibt ende", 1, 0);
	       break;
	  case 7:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: buffer überschreibt alles", 
			    1, 0);
	       break;
	  case -1:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: bytes read < 0", 1, 0);
	       break;
	  case -2:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: bytes read == 0", 1, 0);
	       break;
	  case -3:
	       my_error = 1;
	       my_error_out(error_datei, "io_test: bytes read > buffersize", 
			    1, 0);
	       break;
	  default:
	       my_error = 1;
	       sprintf(string, "io_test: unknown error, read sources "\
		       "returnvalue: %i",ret);
	       my_error_out(error_datei, string, 1, 0);
	       break;

		    
	  }
	  
     }
     ret_thread = pthread_join(thread_1, &thread_result);
     my_output(&my_error);
     close(sockfd);
     close(newsockfd);
     

     /*
      * Consecutively check
      */
     printf ("consecutively check...................................");
     //socket, bind, listen
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error socket(AF_INET, "\
		       "SOCK_STREAM, 0)", errno, 0);
     }else{
	  bzero((char *) &socket_addr, sizeof( socket_addr ) );
	  socket_addr.sin_family = AF_INET;
	  socket_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	  socket_addr.sin_port = htons( TEST_PORT1 ); 
	  if (bind(sockfd, (struct sockaddr *) &socket_addr, 
		   sizeof(socket_addr)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "io_test: Error bind(sockfd, "\
			    "<sockaddr>, sizeof(<sockaddr>))", 
			    errno, 0);
	  }else
	       if (listen(sockfd, 5) < 0){
		    my_error = 1;
		    sprintf(string, "io_test: listen(%i, 5)", sockfd);
		    my_error_out(error_datei, string, errno, 0);
	       }
     }
     fflush(stdout);
     fflush(error_datei);
     ret_thread = pthread_create(&thread_1, NULL, thread_test_ok, NULL);
     if (ret_thread != 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Thread creation failed" , 
		       errno, 0);
     }
     //server
     client_length = sizeof(socket_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &socket_addr, 
			&client_length);
     if(newsockfd < 0){
	  my_error = 1;
	  my_error_out(error_datei, "io_test: Error accept()", errno, 0);
     }else{
	  for (i=0; i<MESSAGESIZE; i++){
	       ret = read_from_client_c(newsockfd);
	       if (ret < 0){
		    my_error = 1;
		    my_error_out(error_datei, "io_test: error occurrence "\
				 "by read(int fd, void *buf, size_t count)", 
				 1, 0);
	       }
	       string_1[i] = ret;
	  }
	  if( strcmp(string_1, MESSAGE) != 0){
	       my_error = 1;
	       sprintf(string, "io_test: strings not equal: \n\t%s\n\t%s", 
		       MESSAGE, string_1);
	       my_error_out(error_datei, string, 1, 0);
	  }
	  
	  
	  
     }
     ret_thread = pthread_join(thread_1, &thread_result);
     my_output(&my_error);
     close(sockfd);
     close(newsockfd);
     return 0;
}






/*
 * the Client Thread
 */
void *thread_test_ok()
{

	  int c_sockfd, ret;
	  int my_error=0;
	  struct sockaddr_in c_socket_addr;

	  if( (c_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "thread_test_ok: Error client "\
			    "socket(AF_INET, SOCK_STREAM, 0)", errno, 0);
	  }else{
	       bzero((char *) &c_socket_addr, sizeof( c_socket_addr ) );
	       c_socket_addr.sin_family = AF_INET;
	       c_socket_addr.sin_addr.s_addr = inet_addr( LOCALHOST );
	       c_socket_addr.sin_port = htons( TEST_PORT1 ); 
	       //sleep(5);
	       if( (ret = connect(c_sockfd, (struct sockaddr *) 
				  &c_socket_addr, sizeof(c_socket_addr))) < 0) {
		    my_error = 1;
		    my_error_out(error_datei, "thread_test_ok: Error "\
				 "connect()", errno, 0);
	       }else{
		    ret = write_to_server(c_sockfd, strlen(MESSAGE), MESSAGE);
		    if (ret < 0){
			 my_error = 1;
			 my_error_out(error_datei, "thread_test_ok: error "\
				      "occurrence by  write(int fd, const "\
				      "char *buf, size_t count)", 1, 0);
		    }
		    close(c_sockfd);
	       }
	  }
	  pthread_exit(&my_error);

	  /* not reached */
	  return NULL;
}







/*
 * read from Client one character
 */
     int read_from_client_c (int fd)
     {
	  char buffer[1];
	  int nbytes;
      
	  nbytes = read (fd, buffer, 1);
	  if (nbytes < 0){
	       exit (EXIT_FAILURE);
	  }else if (nbytes == 0)
	       return 0;
	  else 
	       return (int)*buffer;
     }



/*
 * read from Client and test the buffer
 */
     int read_from_client_buffer (int fd, int size)
     {
	  static char buffer[BUFFSIZE];
	  int nbytes;
	  int i;
	  char *startpos;
	  int ret=0;
     
	  for(i=0; i < BUFFSIZE; i++)
	       buffer[i]= MAGIC_NR;
	  startpos = buffer + (BUFFSIZE/3);
	  nbytes = read (fd, startpos , size);
	  if (nbytes < 0){
	       perror("-1");
	       ret = -1;
	  }else if (nbytes == 0){
	       perror("-2");
	       ret = -2; 
	  }else{ 
	       if (nbytes == size){
		    for(i=0; i < BUFFSIZE; i++){
			 if( (buffer[i] != MAGIC_NR) && (i < BUFFSIZE/3) )
			      ret |= 1;
			 if( (buffer[i] == MAGIC_NR) && (BUFFSIZE/3 <= i) 
			     &&  (i < ((BUFFSIZE/3)+size)))
			      ret |= 2;
			 if( (buffer[i] != MAGIC_NR) 
			     && ( i >= (BUFFSIZE/3)+size) )
			      ret |= 4;
		    }
	       }else if (nbytes < size){
		    for(i=0; i < BUFFSIZE; i++){
			 if( (buffer[i] != MAGIC_NR) && (i < BUFFSIZE/3) )
			      ret |= 1;
			 if( (buffer[i] == MAGIC_NR) && (BUFFSIZE/3 <= i) 
			     &&  (i < ((BUFFSIZE/3)+nbytes)))
			      ret |= 2;
			 if( (buffer[i] != MAGIC_NR) 
			     && ( i >= (BUFFSIZE/3)+size) )
			      ret |= 4;
		    }
	       }else if (nbytes > size){
		    ret = -3;
	       }
	  }
	  return ret;
     }



