/*
 * Main
 */

#include "utils.h"
#include <getopt.h>


struct options
{
     unsigned client :1;
     unsigned server :1;
     char *destination;
     int port;
     unsigned tcp :1;
     unsigned udp :1;
     unsigned help :1;
     unsigned logging :1;
     char *message;
     int package_size;
     int package_count;
     unsigned include_test :1;
  int debug;
}my_options={0};

static char *prog_name;

void usage(char *prog);
void help(char *prog);
int run_program();
int test_ip(char *ip);






void usage(char *prog)
{
     printf("\nUsage: %s [-l] [-c | -s] [-d <ip>] [-u [[-a <size>] [-b <n>]]| -t] \n"\
	    "[-p <port>] [-m <mesg>] [-T] [-h] [-?]\n\n",prog);
}

void help(char *prog)
{
     usage(prog);
     printf("Program for testing the Sockets.\n"	\
	    "                                  : start the test with standart configuration\n"\
	    "-l, --logging                     : logging in error.txt\n"\
	    "-c, --client                      : work as Client\n"\
	    "-s, --server                      : work as Server\n"\
	    "-d, --destination <ip>|<host>     : destination to connect to <xxx.xxx.xxx.xxx> or <host>\n"\
	    "-u, --udp                         : test only udp-Sockets\n"\
	    "-t, --tcp                         : test only tcp-Sockets\n"\
	    "-p, --port  <port>                : test on this port\n"\
	    "-h, --help                        : show this help\n"\
	    "-m, --message <mesg>              : send the message mesg\n"\
	    "-a, --message_size <size>         : send a message with the spezified size in byte (8-65535) default:44\n" \
	    "-b, --package_count <n>           : count the packages default:10\n"\
	    "-T, --include_test                : test the socket function call\n"\
	    "-v, --verbose                     : show many debug code\n"\
	  );
}

/*
 * Main
 */
int main(int argc, char *argv[])
{
     int c;

     prog_name = argv[0];
     if (argc == 1){
	  exit(run_program());
     }
     while(1){
	  int option_index = 0;
	  static struct option long_options[] =
	       {
		    {"client", 0, 0, 0},
		    {"server", 0, 0, 0},
		    {"destination", 1, 0, 0},
		    {"port", 1, 0, 0},
		    {"tcp", 0, 0, 0},
		    {"udp", 0, 0, 0},
		    {"help", 0, 0, 0},
		    {"logging", 0, 0, 0},
		    {"message", 1, 0, 0},
		    {"package_size", 1, 0, 0},
		    {"package_count", 1, 0, 0},
                    {"verbose", 0, 0, 0},
		    {0, 0, 0, 0}
	       };
	  c = getopt_long (argc, argv, "csd:p:tuhlm:a:b:?Tv",
			   long_options, &option_index);
	  if (debug)
	       printf("c:%i optind:%i argc:%i \n",c,optind,argc);
	  if (c == -1){
	       //usage(prog_name);
	       //exit(1);
	       break;
	  }
	  switch (c)
	  {
	       //long_option
	  case 0:
	       if (long_options[option_index].name == "client")
		    my_options.client = 1;
	       if (long_options[option_index].name == "server")
		    my_options.server = 1;
	       if (long_options[option_index].name == "destination")
		    my_options.destination = optarg;
	       if (long_options[option_index].name == "port")
		    my_options.port = str_to_int(optarg);
	       if (long_options[option_index].name == "tcp")
		    my_options.tcp = 1;
	       if (long_options[option_index].name == "udp")
		    my_options.udp = 1;
	       if (long_options[option_index].name == "help")
		    my_options.help = 1;
	       if (long_options[option_index].name == "logging")
		    my_options.logging = 1;
	       if (long_options[option_index].name == "message")
		    my_options.message = optarg;
	       if (long_options[option_index].name == "package_size")
		    my_options.package_size = str_to_int(optarg);
	       if (long_options[option_index].name == "package_count")
		    my_options.package_count = str_to_int(optarg);
	       if (long_options[option_index].name == "include_test")
		    my_options.include_test = 1;
	       if (long_options[option_index].name == "verbose")
		    my_options.debug = 1;
	       
	       break;
	       //short_option
	  case 'c':
	       my_options.client = 1;
	       break;
	  case 's':
	       my_options.server = 1;
	       break;
	  case 'd':
	       my_options.destination = optarg;
	       break;
	  case 'p':
	       my_options.port = str_to_int(optarg);
	       break;
	  case 't':
	       my_options.tcp = 1;
	       break;
	  case 'u':
	       my_options.udp = 1;
	       break;
	  case 'h':
	       my_options.help = 1;
	       break;
	  case 'l':
	       my_options.logging = 1;
	       break;
	  case 'm':
	       my_options.message = optarg;
	       break;
	  case 'a':
	       my_options.package_size = str_to_int(optarg);
	       break;
	  case 'b':
	       my_options.package_count = str_to_int(optarg);
	       break;
	  case '?':
	       my_options.help = 1;;
	       break;
	  case 'T':
	       my_options.include_test = 1;
	       break;
	  case 'v':
	       my_options.debug = 1;
	       break;
	  default:
	       printf ("?? getopt lieferte Zeichcode 0%o zurück ??\n", c);
	  }
     }
     if (optind < argc)
     {
	  usage(prog_name);
	  printf ("Nichtoptionselemente von ARGV: ");
	  while (optind < argc)
	       printf ("%s ", argv[optind++]);
	  printf ("\n");
     }
     exit (run_program());
}







/*
 * run the program 
 */
int run_program()
{
     struct sockaddr_in my_socket_addr_recv; //addr from the receiver
     struct sockaddr_in my_socket_addr_send; //addr from the sender
     int my_test_port;
     int my_package_count; 
     int my_package_size; //message size
     static char my_message[MSG_LEN];
     static char my_ip_addr[HOST_LEN];
     static char my_ip_addr_tmp[HOST_LEN];
     
     
     int my_server_client; // 0-Server+Client, 1-Server, 2-Client
     //set the default message
     my_message[MSG_LEN-1] = 0;
     strncpy(my_message, "abcdefghijklmnopqrstuvwxyz0123456789", MSG_LEN-1); //35 + \0 = 36
     //set the default ip
     my_ip_addr[HOST_LEN-1] = 0;
     strncpy(my_ip_addr, "127.0.0.1", HOST_LEN-1);
     my_ip_addr_tmp[HOST_LEN-1] = 0;
     strncpy(my_ip_addr_tmp, "127.0.0.1", HOST_LEN-1);



     //standards
     bzero((char *) &my_socket_addr_recv, sizeof( my_socket_addr_send ) );
     bzero((char *) &my_socket_addr_send, sizeof( my_socket_addr_recv ) );
     my_package_size = 37; 
     my_package_count = 10;
     my_test_port = 16873;
     my_server_client = 0;
     debug = 0;

     if (my_options.debug)
	  debug = 1;
   

     if (my_options.help){
	  help(prog_name);
	  exit(0);
     }

     if (my_options.logging){
	  error_datei = fopen ("error.txt", "w");
	  if (error_datei == NULL)
	       printf ("coun't create error.txt   %s\n",strerror (errno));
     }else{
	  error_datei = NULL;
     }
     
     
     if (my_options.client && my_options.server){
	  printf("client and server don't work together!\n");
	  usage(prog_name);
	  exit(1);
     }
     
     if (my_options.client && !my_options.destination){
	  printf("There is no IP for server\n");
	  usage(prog_name);
	  exit(1);
     }
     
     

     if ( (my_options.tcp && my_options.udp )
	  || ( my_options.client && my_options.tcp && my_options.udp)
	  || ( my_options.client && !my_options.tcp && !my_options.udp)){
	  printf("the server or client without TCP or UDP dosn't work\n");
       	  usage(prog_name);
	  exit(1);
     }




     if (!my_options.client && my_options.destination){
	  printf("a destination without the Option \"--client or -c\" "\
		 "dosn't work\n");
	  usage(prog_name);
	  exit(1);
     }
     

     if (my_options.tcp && my_options.package_count ){
	  printf("tcp and package count  don't work together\n"\
		 "package count are ignored!\n");
	  my_options.package_count = 0;
     }
     
     if (my_options.port)
	  my_test_port = my_options.port;
     
     if (my_options.message){
	  strncpy(my_message, my_options.message, MSG_LEN-1);
	  my_package_size = strlen(my_message);
     }
     
     if (my_options.package_count)
	  my_package_count = my_options.package_count;

     if (my_options.destination){
	  strncpy(my_ip_addr, my_options.destination, HOST_LEN-1);
	  if(isalpha(my_ip_addr[0])){
	       printf("hostname\n");
	       if(!addr_to_ip(my_ip_addr, my_ip_addr_tmp, HOST_LEN-1)){
		    printf("unknown host!\n");
		    exit(1);
	       }
	       strncpy(my_ip_addr, my_ip_addr_tmp, HOST_LEN-1);
	  }else{
	       printf("IP\n");
	       strcpy(my_ip_addr_tmp, my_ip_addr); //copy
	       if (!str_to_ip(my_ip_addr_tmp))
		    exit(1);
	  }
     }
     

     if (my_options.package_size){
	  if ((my_options.package_size > 7) 
	      && ( my_options.package_size < 65536)){
	       char str[my_package_size];
	       my_package_size = my_options.package_size;
	       strcpy(str, my_message);
	       padding_1(str, my_package_size+1);
	       strcpy(my_message, str);
	  }else
	       printf("wrong package size\nthe size must be between: "\
		      "7 < %i < 65536\n", my_options.package_size);
     }

    
          
     
     //start here
     printf ("\n************************\n");
     printf ("* Socket Test Programm *\n");
     printf ("************************\n");	  
     if (my_options.include_test){
	  if (socket_test ())
	       return 1;
	  if (bind_test ())
	       return 1;
	  if(listen_test())
	       return 1;
	  if (accept_test ())
	       return 1;
	  if (close_shutdown_test())
	       return 1;
	  if (io_test())
	       return 1;
     }
     if(debug)
	  printf("port:%i\nserver_client:%i\npackage_size:%i\nip_addr:%s"\
		 "\nmessage:%s\npackage_count:%i\n",
		 my_test_port, my_server_client, my_package_size, my_ip_addr, 
		 my_message, my_package_count);
     

     if (my_options.client){
	  //nur Client
	  my_server_client = 2;
	  if (my_options.tcp){
	       //tcp
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	  }else if (my_options.udp){
	       //udp
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }else{
	       //beides zusammen
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }
     }else if (my_options.server){
	  //nur Server
	  my_server_client = 1;
	  if (my_options.tcp){
	       //tcp
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	  }else if (my_options.udp){
	       //udp
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }else{
	       //beides zusammen
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }
     }else{
	  //beides zusammen
	  my_server_client = 0;
	  if (my_options.tcp){
	       //tcp
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	  }else if (my_options.udp){
	       //udp
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }else{
	       //beides zusammen
	       io_test_tcp(my_server_client, my_test_port, my_package_size,
			   my_ip_addr, my_message);
	       io_test_udp(my_package_size, my_test_port, my_package_count, 
			   my_ip_addr, my_server_client, my_message);
	  }
     }
     
     if (debug)
	  printf("\nport:%i\nmessage:%s\npackage_count:%i\npackage_size:%i\n",
		 my_test_port,my_message,my_package_count,my_package_size);

     //close fd when it open
     if (my_options.logging)
	  fclose(error_datei);

     return 0;
}



