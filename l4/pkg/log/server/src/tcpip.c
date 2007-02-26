/*!
 * \file	log/server/src/tcpip.c
 * \brief	TCP-based Server to get the logging via TCP/IP
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This file includes the code for handling remote TCP/IP-requests.
 *
 * With the current OSKit-Implementation, we only support single-threaded
 * OSKIT-Kernels. This requires all the network handling to be done in one
 * thread.
 *
 * In detail, this file handles:
 * - dealing with the TCP/IP-functions
 * - implementing the net-flush-operation
 *
 * Function-Interface:	net_init(), net_wait_for_client(),
 *			net_receive_check(), net_flush_buffer().
 *
 * The network connection can operate in two modes: in standard logging mode
 * and in multiplexed mode. The standard logging mode transfers the logged
 * data unmodified to the client. The multiplexed mode allows to additionally
 * send binary data on binary channels. The multiplexed mode has a different
 * line format to encapsulate the different channels. As a result, the
 * logging data must be modified to fit in the same channel.
 *
 * The standard mode and the multiplexed mode connection may be
 * configured to use the same TCP-port. But they also can use
 * different TCP-ports. In standard mode, no binary data will be transfered.
 * Instead, a warning will show up in the logging-output the first few times
 * binary data was tried to send. The LOG_channel_open() at the client
 * will return -L4_EBUSY in this case.
 * 
 * This file requires: do_flush_buffer() */

#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/error.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/clientos.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>
#include <oskit/net/bootp.h>
#include <malloc.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include "config.h"
#include "tcpip.h"
#include "flusher.h"
#include "stuff.h"
#include "muxed.h"

#if CONFIG_USE_TCPIP==0
#error This file should not be compiled if tcpip-networking is disabled
#endif

#define IFNAME		"eth0"

char *ip_addr=NULL;
char *gateway=NULL;
char *netmask=NULL;

/* variable to hold the socket creator function */
oskit_socket_factory_t *socket_create;
static oskit_socket_t *acceptor_socket=0;
oskit_socket_t *client_socket = 0;

static int net_send(void*addr, int size);
static int muxed_write(int (*fn)(void*addr, int size),
		       unsigned char channel, void*addr, unsigned size);


#define INITDRV(drv) if(oskit_linux_init_ethernet_##drv())	\
                     LOG_Error("Error initalizing etherdriver %s.\n",#drv);

/*!\initialize some ethernet drivers.
 *
 * \return 0 on success
 */
static int init_ethernet(void){
//    INITDRV(ne);
//    INITDRV(eepro100);
    INITDRV(de4x5);
//    INITDRV(wd);
//    INITDRV(tulip);
    return 0;
}

/*
 * Initialize the network adapter and create the acceptor socket in it.
 *
 * \return 0 on success, error otherwise.
 */
int net_init(int argc, char **argv){
	struct oskit_freebsd_net_ether_if *eif;
	oskit_etherdev_t **etherdev;
	oskit_etherdev_t *dev;
	int err, ndev;
	oskit_osenv_t *osenv;
	struct sockaddr_in server;

	oskit_clientos_init();

	printf("Initializing devices...\n");
	osenv = oskit_osenv_create_default();
	oskit_register(&oskit_osenv_iid, (void *) osenv);

	// create (empty) service-databases for oskit device driver support
	oskit_dev_init(osenv);
	oskit_linux_init_osenv(osenv);

	if((err=init_ethernet())!=0) return err;

	printf("Probing devices...\n");
	oskit_dev_probe();

	printf("Probing done.\n");
	oskit_dump_devices();

	printf("Initializing network code...\n");
	/* initialize network code */
	err = oskit_freebsd_net_init(osenv, &socket_create);	
	assert(!err);

	/*
	 * Find all the Ethernet device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0){
	    printf("No Ethernet adaptors found\n");
	    goto error;
	}

	/*
	 * for now, we just go for the first device...
	 */
	printf("Using the first device\n");
	dev = etherdev[0];

	if(ip_addr==NULL){
	    struct bootp_net_info bootp_info;

	    /* get the IP address & other info */
	    err = bootp_gen(dev, &bootp_info, CONFIG_BOOTP_RETRIES,
			    CONFIG_BOOTP_TIMEOUT);
	    if(err){
		printf("Got no reply to my bootp-request.\n");
		goto error;
	    }
	    if(!bootp_info.flags & BOOTP_NET_IP){
		printf("Got no IP address on bootp-request.\n");
		err=OSKIT_ENOTSUP;
		goto error_free;
	    }
	    if(!bootp_info.flags & BOOTP_NET_GATEWAY){
		printf("Got no IP gateway on bootp-request.\n");
		err=OSKIT_ENOTSUP;
		goto error_free;
	    }
	    if(!bootp_info.flags & BOOTP_NET_NETMASK){
		printf("Got no IP netmask on bootp-request.\n");
		err=OSKIT_ENOTSUP;
		goto error_free;
	    }

	    if(((ip_addr=(char*)malloc(20))==NULL) ||
	       ((netmask=(char*)malloc(20))==NULL) ||
	       ((gateway=(char*)malloc(20))==NULL)){
		LOG_Error("Not enough memory.\n");
		err=OSKIT_ENOMEM;
		goto error_free;
	    }
	    strcpy(ip_addr, inet_ntoa(bootp_info.ip));
	    strcpy(netmask, inet_ntoa(bootp_info.netmask));
	    strcpy(gateway, inet_ntoa(bootp_info.gateway.addr[0]));

	  error_free:
	    bootp_free(&bootp_info);
	    if(err) return err;
	}
	/*
	 * Open first one; could use oskit_freebsd_net_open_first_ether_if,
	 * but we already have a handle on that one.
	 */
	err = oskit_freebsd_net_open_ether_if(dev, &eif);
	if(err){
	    LOG_Error("oskit_freebsd_net_open_ether_if() returned error %#x\n",
		  err);
	    goto error;
	}

	/* configure the interface */
	err = oskit_freebsd_net_ifconfig(eif, IFNAME,
					 ip_addr,
					 netmask);
	if(err){
	    printf("Error during ifconfig to interface\n");
	    goto error;
	}

	if(gateway){
	    err = oskit_freebsd_net_add_default_route(gateway);
	    if (err){
		printf("couldn't add default route (%d)\n", err);
	    }
	}
	

	/* We have the network adapter up and running, create
	   and bind the listener-socket now. */

	if((err = oskit_socket_factory_create(socket_create, OSKIT_AF_INET,
					      OSKIT_SOCK_STREAM,
					      IPPROTO_TCP,
					      &acceptor_socket))!=0){
	    LOG_Error("socket() returned error %#x\n", err);
	    goto error;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family=OSKIT_AF_INET;
	server.sin_port=htons(PORT_NR);
	server.sin_addr.s_addr=htonl(INADDR_ANY);

	if((err=oskit_socket_bind(acceptor_socket,
				  (struct oskit_sockaddr*)&server,
				  sizeof(server)))!=0){
	    LOG_Error("bind() returned error %#x.\n", err);
	    goto error;
	}
	LOGd(CONFIG_LOG_TCPIP,
		"Bound to port %d\n", ntohs(server.sin_port));
	if((err=oskit_socket_listen(acceptor_socket,1))!=0){
	    LOG_Error("listen() returned error %#x.\n", err);
	    goto error;
	}

	return 0;	// ok

  error:
	/* close etherdev and release net_io devices */
	oskit_freebsd_net_close_ether_if(eif);
	return err;
}

/*!\brief Deal with error during operation on opened client-connection.
 *
 * This function closes the socket and sets client_socket to 0.
 */
static void net_error_connected(void){
    int err;

    if(!flush_to_net) return;
    assert(client_socket);

    LOGd(CONFIG_LOG_TCPIP | CONFIG_LOG_NOTICE,
	"Closing client-connection\n");
    err = oskit_socket_release(client_socket);
    if(err){
	LOG_Error("error %#x closing the faulty client-socket\n",
	       err);
    }
    client_socket = 0;
}

/*!\brief Wait for a new connection and assign it to client_socket.
 *
 * This function blocks.
 *
 * \param socket	pointer to listener-socket
 *
 * \return	        0 on success
 */
int net_wait_for_client(void){
    int len;
    oskit_socket_t *ns;
    struct sockaddr_in client;    /* client address information */
    struct timeval time;
    int err;
    char msgbuffer[130];

    if(acceptor_socket==0) return OSKIT_ENOTCONN;

    assert(client_socket==0);

    len = sizeof(client);
    oskit_socket_getsockname(acceptor_socket,
			     (struct oskit_sockaddr*)&client,&len);
    LOGd(CONFIG_LOG_TCPIP | CONFIG_LOG_NOTICE,
	 "Waiting for a client to connect to %s:%u\n",
	 ip_addr, ntohs(client.sin_port));

    client.sin_family=OSKIT_AF_INET;
    client.sin_port=htons(INADDR_ANY);
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    len=sizeof(client);
    while((err=oskit_socket_accept(acceptor_socket,
				   (struct oskit_sockaddr*)&client,
				   &len, &ns))==EINTR);
    if(err){
	LOG_Error("oskit_socket_accept() returned error %#x\n", err);
	return err;
    }

    client_socket = ns;

    if((err=oskit_socket_getpeername(client_socket,
				     (struct oskit_sockaddr*)&client,
				     &len))!=0){
	LOG_Error("oskit_socket_getpeername() returned error %#x\n", err);
	net_error_connected();
	return err;
    }

    LOGd(CONFIG_LOG_TCPIP | CONFIG_LOG_NOTICE,
	   "Client %ld.%ld.%ld.%ld:%d connected.\n",
	   (ntohl(client.sin_addr.s_addr)>>24)&0xff,
	   (ntohl(client.sin_addr.s_addr)>>16)&0xff,
	   (ntohl(client.sin_addr.s_addr)>> 8)&0xff,
	   (ntohl(client.sin_addr.s_addr)>> 0)&0xff,
	   ntohs(client.sin_port));

    /* set timeout value on client-socket to a few milliseconds */
    time.tv_sec = 0;
    time.tv_usec = 10000;	// 10ms in minimum, more is possible
    err = oskit_socket_setsockopt(client_socket, OSKIT_SOL_SOCKET,
				  OSKIT_SO_RCVTIMEO, &time, sizeof(time));
    if(err){
	LOG_Error("oskit_socket_setsockopt() returned error %#x\n", err);
	net_error_connected();
	return err;
    }

#if 0
    {
	int val=40000;

	if((err=oskit_socket_setsockopt(client_socket, OSKIT_SOL_SOCKET,
				       OSKIT_SO_SNDBUF, &val, sizeof(val)))!=0)
	    LOG_Error("oskit_socket_setsockopt() returned error %#x setting the sendbuffersize to %d\n", err, val);
	if((err=oskit_socket_setsockopt(client_socket, OSKIT_SOL_SOCKET,
				       OSKIT_SO_RCVBUF, &val, sizeof(val)))!=0)
	    LOG_Error("oskit_socket_setsockopt() returned error %#x setting the recvbuffersize to %d\n", err, val);
    }
#endif

    sprintf(msgbuffer,
	    "\nWelcome to the L4-Logserver.\n"
	    "Using a Buffersize of %d. Log follows.\n\n", buffer_size);
    /* Send a hello-message */
    if(flush_muxed == 0) {
	err = net_send(msgbuffer, strlen(msgbuffer)+1);
    } else {
	err = muxed_write(net_send, 1,
			  msgbuffer, strlen(msgbuffer)+1);
    }
    if(err){
	LOG_Error("Error %#x sending hello-message, closing connection\n", err);
	net_error_connected();
	return err;
    }

    return 0;
}

/*!\brief Check if data is available on the client-socket.
 *
 * \retval 0	if the client-connection is ok.
 * \retval !0   client-connection closed, caller should wait for the next
 *              client
 *
 * If the client sent data we interpret this as a request for flushing
 * our own buffer. This way, a user could sent a character (e.g., by
 * hitting his keyboard nervously on a two-way connection) to get the
 * buffered data.
 */
int net_receive_check(void){
    struct oskit_sockaddr dummy_addr;
    unsigned dummy_len, len;
    int err;
    char buf[10];

    if(!client_socket) return OSKIT_ENOTCONN;
		
    dummy_len = sizeof(dummy_addr);
    if((err=oskit_socket_recvfrom(client_socket,
				  buf,sizeof(buf)-1,
				  0,
				  &dummy_addr, &dummy_len,
				  &len))){
	if(err == OSKIT_EWOULDBLOCK	||
	   err == OSKIT_EAGAIN){
	    LOGd(CONFIG_LOG_TCPIP, "No data on socket.\n");
	    return 0;
	}
	LOGd(CONFIG_LOG_TCPIP || 
		(err!=OSKIT_EPIPE && err!=OSKIT_ECONNRESET),
		"oskit_socket_recvfrom() returned error %#x\n",
		err);
	net_error_connected();
	l4_sleep(1000);
	return err;
    }
    buf[len]=0;
    if(len){
	LOGd(CONFIG_LOG_TCPIP, "Received %d Byte, %s, flushing output\n",
		len,buf);
	/* flush the buffers */
	do_flush_buffer();
    } else {
	net_error_connected();
	return OSKIT_ENOTCONN;
    }
    return 0;
}

/*!\brief Send data to the network
 *
 * This function sends data to the socket in 'client_socket'.
 */
static int net_send(void*addr, int size){
    int err, len;

    if(client_socket == 0) return OSKIT_ENOTCONN;

    if((err=oskit_socket_sendto(client_socket,
				addr, size, 0,
				NULL, 0, &len))!=0){
	LOGd(CONFIG_LOG_TCPIP||err!=OSKIT_EPIPE,
		"send () returned error %#x.\n", err);
	net_error_connected();
    }
    return err?err!=OSKIT_EPIPE?err:0:0;
}

static int muxed_write(int (*fn)(void*addr, int size),
		       unsigned char channel, void*addr, unsigned size){
    struct muxed_header header;
    char ret;

    header.version      = LOG_PROTOCOL_VERSION;
    header.channel      = channel;
    header.type         = LOG_TYPE_LOG;
    header.flags        = LOG_FLAG_NONE;
    header.data_length  = size;
    if((ret = fn(&header, sizeof(header)))<0) return ret;
    return fn(addr, size);
}

/*!\brief Flush the buffers to the socket in 'client_socket'.
 *
 * This function flushes the message buffer. If we are in muxed mode
 * (flush_muxed=1), we also flush the binary buffers.
 *
 * \retval	0 on success, error otherwise.
 */
int net_flush_buffer(char*addr, int size){
    int err, i;

    if(flush_muxed == 0) {
	/* normal output mode */
	return net_send(addr, size);
    } else {
	/* muxed output mode */
	if((err = muxed_write(net_send, 1, addr, size))!=0) return err;

	/* flush the binary buffers */
	for(i=0; i<MAX_BIN_CONNS; i++){
	    if(bin_conns[i].channel &&
	       bin_conns[i].flushed < bin_conns[i].written){
		muxed_write(net_send, bin_conns[i].channel,
			    bin_conns[i].addr, bin_conns[i].size);
		bin_conns[i].flushed++;
	    }
	}
	return 0;
    }
    return OSKIT_ENOTCONN;
}

