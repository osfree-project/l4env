#include "utils.h"

/*
 * Der Socket Test
 */

int type_protocol (int j, int k, int k_end, int protocol, 
		   int pro_ende, int fehler);


/*
 * test the socket command
 * int socket(int domain, int type, int protocol);
 */
int socket_test()
{
	int i = 0;
	int sockfd;
	int j;
	int my_error=0;
	char string[2048];
	struct socket_type my_socket[30] =
	{
		{1,"PF_LOCAL", 1, "SOCK_STREAM",0,"IPPROTO_IP"},
		{1,"PF_LOCAL", 1, "SOCK_STREAM", 1,"IPPROTO_ICMP"},
		{1,"PF_LOCAL", 2, "SOCK_DGRAM", 0,"IPPROTO_IP"},
		{1,"PF_LOCAL", 2, "SOCK_DGRAM", 1,"IPPROTO_ICMP"},
		{1,"PF_LOCAL", 3, "SOCK_RAW", 0,"IPPROTO_IP"},
		{1,"PF_LOCAL", 3, "SOCK_RAW", 1,"IPPROTO_ICMP"},
		{2,"PF_INET", 1, "SOCK_STREAM", 0,"IPPROTO_IP"},
		{2,"PF_INET", 1, "SOCK_STREAM", 6,"IPPROTO_TCP"},
		{2,"PF_INET", 2, "SOCK_DGRAM", 0,"IPPROTO_IP"},
		{2,"PF_INET", 2, "SOCK_DGRAM", 17,"IPPROTO_UDP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 0,"IPPROTO_IP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 1,"IPPROTO_ICMP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 2,"IPPROTO_IGMP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 4,"IPPROTO_IPIP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 6,"IPPROTO_TCP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 8,"IPPROTO_EGP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 12,"IPPROTO_PUP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 17,"IPPROTO_UDP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 22,"IPPROTO_IDP"},
		{16,"PF_NETLINK", 2, "SOCK_DGRAM", 29,"IPPROTO_XXX"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 0,"IPPROTO_IP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 1,"IPPROTO_ICMP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 2,"IPPROTO_IGMP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 4,"IPPROTO_IPIP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 6,"IPPROTO_TCP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 8,"IPPROTO_EGP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 12,"IPPROTO_PUP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 17,"IPPROTO_UDP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 22,"IPPROTO_IDP"},
		{16,"PF_NETLINK", 3, "SOCK_RAW", 29,"IPPROTO_XXX"}
	};

	/*
	 * All working combinations
	 */
	printf ("\n\nsocket_test()\n");
	printf ("all working combinations..............................");
	for (i = 0; i < 30; i++) {
		sockfd = socket (my_socket[i].domain_i, my_socket[i].type_i, 
				 my_socket[i].protocol_i);
		if (sockfd < 0) {
			sprintf(string,"socket_test: socket( %s, %s, %s)", 
				my_socket[i].domain_c, 
			 my_socket[i].type_c, my_socket[i].protocol_c);
			my_error_out(error_datei, string, errno, 0);
			my_error = 1;
		}
	}
	my_output(&my_error);

	/*
	 * Felmeldung: Address family not supported by protocol 97
	 */
	printf ("Error: Address family not supported by protocol.......");
	for (j = 3; j < 5; j++){			
		my_error = type_protocol (j, 0, 11, 0, 128, 97);
	}
	my_output(&my_error);

	/*
	 * Fehlermeldung: Socket type not supported 94
	 */
	printf ("Error: Socket type not supported......................");
	my_error = type_protocol (16, 4, 11, 0, 255, 94);
	my_output(&my_error);

	/*
	 * Fehlermeldung: Protocol not supported 93
	 */
	printf ("Error: Protocol not supported.........................");
	my_error = type_protocol (16, 2, 4, 41, 255, 93);
	my_output(&my_error);

	/*
	 * Fehlermeldung: Operation not permitted 1
	 */
	printf ("Error: Operation not permitted........................");
	my_error = type_protocol (17, 0, 11, 0, 255, 1);
	my_output(&my_error);

	/*
	 * Fehlermeldung: Invalid argument 22
	 */
	printf ("Error: Invalid argument...............................");
	for (i = 0; i < 32; i++)
		my_error = type_protocol (i, 11, 32, 0, 255, 22);
	my_output(&my_error);
	close(sockfd);
	return 0;
}

/*
 * j socket_family
 * k socket_type anfang
 * k_end socket_type ende
 * fehler zu erwartender Fehler
 */
int type_protocol (int j, int k, int k_end, int protocol, int pro_ende, 
		   int fehler){
	int sockfd;
	int ret=0;
	char string[2048];

	for (; k < k_end; k++){			//11 sonst Invalid argument 22 
		for (; protocol < pro_ende; protocol++) {		//255
			sockfd = socket (j, k, protocol);
			if (errno != fehler || sockfd >= 0){
				ret = 1;
				sprintf(string,"socket: socket(%i,%i,%i)", 
					j, k, protocol);
				my_error_out(error_datei, string, errno, fehler);
			}
		}
	}
	return ret;
}

