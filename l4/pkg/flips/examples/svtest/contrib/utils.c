#include "utils.h"

#include <stdio.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>


double getSec();



void my_output(int *status){
     if(*status)
	  printf ("Error\n");
     else
	  printf("OK\n");
     *status=0;
}



/*
 * ausgabe in datei mit filedescriptor
 * ausgabe in stder mit NULL
 */
void my_error_out(FILE *error_datei, const char *description, 
		  int error_number, int error_expected) {
     FILE *fd = error_datei ? error_datei : stderr;
     
     fprintf(fd, "\n%s\n\tError: %8i %s\n\tExpected: %5i %s\n", 
	     description, error_number,
	     strerror(error_number), 
	     error_expected, strerror(error_expected));
}



/*
 * ermittelt die Zeit
 */
double getSec(){
	struct timeval tv;
	struct timezone tz;
	double erg;
	gettimeofday (&tv, &tz);
	erg = (double) tv.tv_sec;
	erg += (double) tv.tv_usec / 1000000.0f;
	return erg;
}

/*
 * socket
#define MY_TYPE_TCP SOCK_STREAM 
#define MY_TYPE_UDP SOCK_DGRAM
 */
int my_socket(int type)
{
     int my_sockfd;
     const char * msg;
     assert( type == MY_TYPE_TCP || type ==  MY_TYPE_UDP);
     
     msg = (type ==  MY_TYPE_TCP) ? 
	  "utils: Error socket(MY_FAMILY, MY_TYPE_TCP, MY_PROTOCOL)" :
	  "utils: Error socket(MY_FAMILY, MY_TYPE_UDP, MY_PROTOCOL)";
     
     if( (my_sockfd = socket(MY_FAMILY, type, MY_PROTOCOL)) < 0) {
	  my_error_out(error_datei, msg, errno, 0);
	  return 0;
     }

     return my_sockfd;
}

int my_bind_server(int my_sockfd, int port, 
		   struct sockaddr_in my_socket_addr_recv)
{
     bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_recv ) );
     my_socket_addr_recv.sin_family = MY_FAMILY;
     my_socket_addr_recv.sin_addr.s_addr = htonl( INADDR_ANY );
     my_socket_addr_recv.sin_port = htons( port );
     /*
     my_socket_addr_recv->sin_family = MY_FAMILY;
     my_socket_addr_recv->sin_port = htons( port ); 
     my_socket_addr_recv->sin_addr.s_addr = INADDR_ANY;
     */
     if (bind(my_sockfd, (struct sockaddr *) &my_socket_addr_recv, 
	      sizeof(my_socket_addr_recv)) < 0) {
       my_error_out(error_datei, 
		    "utils: Error bind_server(sockfd, <sockaddr_recv>, "\
		    "sizeof(<sockaddr_recv>))", 
		    errno, 0);  
       if (debug)
	 printf("utils: my_bind_server:sockfd:%i\n",my_sockfd);
       return 0;
     }
     return 1;
}

int my_bind_client(int my_sockfd, int port, 
		   struct sockaddr_in *my_socket_addr_recv, 
		   struct sockaddr_in *my_socket_addr_send, 
		   const char *my_ip_addr)
{
  if (debug)
    printf("utils: my_bind_client: ip:%s\n",my_ip_addr);

     my_socket_addr_recv->sin_family = MY_FAMILY;
     my_socket_addr_recv->sin_port = htons( port ); 
     my_socket_addr_recv->sin_addr.s_addr = inet_addr(my_ip_addr);
     my_socket_addr_send->sin_family = MY_FAMILY;
     my_socket_addr_send->sin_addr.s_addr = htonl( INADDR_ANY );
     my_socket_addr_send->sin_port = htons( 0 ); 	
     
     if (bind(my_sockfd, (struct sockaddr *) my_socket_addr_send, 
	      sizeof(my_socket_addr_send)) < 0){
	  my_error_out(error_datei, 
		       "utils: Error bind_client(sockfd, <sockaddr_send>, "\
		       "sizeof(<sockaddr_send>))", 
		       errno, 0);  
	  return 0;
     }
     return 1;
}


/*
 * bind
 * port Port for Connection
 */

#if 0
int my_bind(int port)
{
     int my_error = 0;
     

     //bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_send ) );
     //bzero((char *) &my_socket_addr_send, sizeof( my_socket_addr_recv ) );
     my_socket_addr_recv.sin_family = MY_FAMILY;
     my_socket_addr_recv.sin_port = htons( port ); 
     my_socket_addr_send.sin_family = MY_FAMILY;
     my_socket_addr_send.sin_addr.s_addr = htonl( INADDR_ANY );
     my_socket_addr_send.sin_port = htons( 0 ); 	

     if (my_server_client == 2){//client
	  my_socket_addr_recv.sin_addr.s_addr = inet_addr(my_ip_addr);
	  if (bind(my_sockfd, (struct sockaddr *) &my_socket_addr_send, 
		   sizeof(my_socket_addr_send)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "utils: Error bind(sockfd, "\
			    "<sockaddr_send>, sizeof(<sockaddr_send>))", 
			    errno, 0);  
	       return 0;
	  }
     }else if (my_server_client == 1){//server
	  my_socket_addr_recv.sin_addr.s_addr = INADDR_ANY;
	  if (bind(my_sockfd, (struct sockaddr *) &my_socket_addr_recv, 
		   sizeof(my_socket_addr_recv)) < 0){
	       my_error = 1;
	       my_error_out(error_datei, "utils: Error bind(sockfd, "\
			    "<sockaddr_recv>, sizeof(<sockaddr_recv>))", 
			    errno, 0);  
	       return 0;
	  }
     }else{//server + client
	  //printf("bind:beides\n");

     }

     return 1;
}
#endif
/*
 * listen
 */
int my_listen(int my_sockfd)
{
     char string [2048];
     
     if (listen(my_sockfd, 5) < 0){
	  sprintf(string, "utils: Error listen(%i, 5)", my_sockfd);
	  my_error_out(error_datei, string, errno, 0);
	  return 0;
     }
     return 1;
}


//IP-Adresse xxx.xxx.xxx.xxx einlesen (IPv4)
int str_to_ip(char *str)
{
     char *tmp;
     char trennzeichen[] = ".";
     int i=0;
               
     tmp = strtok(str, trennzeichen);
     while (tmp !=  NULL){
	  if ((0 <= str_to_int(tmp)) && (str_to_int(tmp) <= 255))
	       // printf(" %s\n",tmp);
	       ;
	  else{
	       printf("wrong IP - the right format is xxx.xxx.xxx.xxx\n");
	       return 0;
	  }
	  tmp = strtok(NULL, trennzeichen);
	  i++;
     }
     if ( i == 4)
	  return 1;
     else{
	  printf("wrong IP - the right format is xxx.xxx.xxx.xxx\n");
	  return 0;
     }
}


//string to int
int str_to_int(char *string)
{
     long ret;
     char *end;
     ret = strtol(string, &end, 10);
     if(*end != '\0'){
	  printf("invalid number!   %s \n",end);
	  //usage(prog_name);
	  exit(1);
	  //return (-1);
     }
     return ret;
}

//string auffüllen
void padding(char * tmp,const char * string,const char * key) {
     char *t;
     const char *s,*k;

     for (s=string,t=tmp,k=key; *s; ++s,++t) {
	  *t=*k;
	  if (!*++k)
	       k=key;
     }
     *t = 0;
}    

/*
void padding(char * tmp,const char * string) {
     char *t;
     const char *s;
     
     printf("%i\n",strlen(tmp));
     

     for (s=string,t=tmp; *s; ++s,++t) {
	  *t=*s;
	  if (!*++s)
	       s=string;
     }
     *t = 0;
}    
*/



void test(char * string, int size)
{
     char tmp[size];
     
     strcpy(tmp,string);
     if (strlen(string) < size){
	  while(strlen(tmp)< size){
	       
	       strncat(tmp, string, (size - strlen(string)- 1));
	       printf("size:%i  tmp:>%s<  string:>%s< strlen(tmp):%i\n", 
		      size, tmp, string, strlen(tmp));
	  }
	  
     }
}


/** Pad string.
 * param s        a null-terminated string.
 * param pad_size size of memory pointed-to by s, including null byte. 
 */
void padding_1(char* s, size_t pad_size)
{
     int i = 0;
     int j = 0;
     size_t len = strlen(s);
     j = len;
     
     while (len < pad_size-1){
	  s[len++] = s[i%j];
	  i++;
     }
     s[len] = 0;
}


/*
 * read from Client
 */
int read_from_client (int fd, int size)
{
     char buffer[size];
     int nbytes;
      
     nbytes = read (fd, buffer, size);
     if (debug)
       printf("utils: read_from_client: nbytes:%i\n", nbytes);
	  
     if (nbytes < 0)
	  exit (EXIT_FAILURE);
     else
	  return 0;
     
     
}
/*
 * read from Client
 */
int read_from_client_buf (int fd, int size, char buffer[])
{
     //char buffer[size];
     int nbytes;
      
     nbytes = read (fd, buffer, size);
     if (debug)
       printf("utils: read_from_client: nbytes:%i\n", nbytes);
	  
     if (nbytes < 0)
	  exit (EXIT_FAILURE);
     else
	  return nbytes;
     
     
}


/*
 * write to Server
 */
int write_to_server (int fd, int size, const char *my_message)
{
     int nbytes;
     
     if (debug)
       printf("utils: write_to_server: fd:%i size:%i message:%s\n", 
	      fd, size, my_message);
     nbytes = write (fd, my_message, size);
     if (debug)
       printf("utils: write_to_server: %i bytes writen\n",nbytes);
     if (nbytes < 0){
	  my_error_out(error_datei, "utils: Error by write_to_server", 
		       errno, 0);  	  
	  return 0;
     }else
	  return nbytes;
}


/**
 * Address to IP
 *
 * host is the hoatname
 * out return the ip
 * size the length of host
 */
int addr_to_ip(const char *host, char *out, int size)
{
     struct hostent *host_info;
     char **host_addrs;
  
     host_info = gethostbyname(host);
     if ( !host_info){
	  my_error_out(error_datei, "utils: Error by gethostbyname()", errno, 0);
	  return 0;
     }else{
	  host_addrs = host_info -> h_addr_list;
	  if (debug)
	    printf("utils: addr_to_ip: %s:%i\n",
		   inet_ntoa(*(struct in_addr *)*host_addrs), size);
	  strncpy(out, inet_ntoa(*(struct in_addr *)*host_addrs), size-1);
	  if (debug)
	    printf("utils: addr_to_ip: %s\n",out);
     }
     return 1;
}
