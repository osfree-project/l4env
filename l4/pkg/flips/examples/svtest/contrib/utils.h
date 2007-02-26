#ifndef __UTILS
#define __UTILS


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>

#define TEST_PORT 16873
#define TEST_PORT1 24537
#define LOCALHOST "127.0.0.1"
//standard
#define MY_FAMILY AF_INET
#define MY_TYPE_TCP SOCK_STREAM 
#define MY_TYPE_UDP SOCK_DGRAM
#define MY_PROTOCOL 0
#define MY_TCP 1
#define MY_UDP 0
/* Typ fuer 32-Bit IP-Addresse  */
#define IPADDR unsigned long
#define MSG_LEN 65535 //max mesg length
#define HOST_LEN 1024 //max length of ip or hostname


FILE *error_datei; //if not set, output to stderr
int debug;

//functions
void my_output(int *status);
void my_error_out(FILE *error_datei, const char *description, 
		  int error_number, int error_expected);
double getSec ();
int socket_test();
int listen_test();
int bind_test();
int accept_test();
int io_test();
int io_test_tcp();
int io_test_udp();
int close_shutdown_test();
int my_socket(int type);
int my_bind(int port);
int my_listen(int my_sockfd);
int str_to_ip(char *str);
int str_to_int(char *string);
void padding(char * tmp,const char * string,const char * key);
void padding_1(char* s, size_t pad_size);
int read_from_client (int fd, int size);
int read_from_client_buf (int fd, int size, char buffer[]);
int write_to_server (int fd, int size, const char *my_message);

int addr_to_ip(const char *host, char *out, int size);
int my_bind_server(int my_sockfd, int port, 
		   struct sockaddr_in my_socket_addr_recv);
int my_bind_client(int my_sockfd, int port, 
		   struct sockaddr_in *my_socket_addr_recv, 
		   struct sockaddr_in *my_socket_addr_send, 
		   const char *my_ip_addr);




struct socket_type
{
     int domain_i;
     char domain_c[20];
     int type_i;
     char type_c[20];
     int protocol_i;
     char protocol_c[20];
};

     
#endif
