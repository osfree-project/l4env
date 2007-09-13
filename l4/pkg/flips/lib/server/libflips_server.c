/*** GENERAL INCLUDES ***/
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>

/*** LOCAL INCLUDES ***/
#include "flips-server.h"

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

/* !!! here some security should be added .-) */
#define client_owns_sid(c,s) 1

extern void flips_session_thread(void *arg);

/** IDL INTERFACE: START SOCKET SESSION
 */
l4_threadid_t l4vfs_connection_init_connection_component(CORBA_Object _dice_corba_obj,
                                               CORBA_Server_Environment * _dice_corba_env)
{
	l4_threadid_t srv;

	/* XXX error handling is not bad */
	l4thread_create((l4thread_fn_t) flips_session_thread,
	                 &srv, L4THREAD_CREATE_SYNC);
	return srv;
}

/** IDL INTERFACE: CLOSE SOCKET SESSION
 */
void l4vfs_connection_close_connection_component(CORBA_Object _dice_corba_obj,
                                                 const l4_threadid_t *server,
                                                 CORBA_Server_Environment * _dice_corba_env)
{
	LOGd(_DEBUG, "Flips(close_session): put session thread to death - fare well.");
	l4thread_exit();
}

/** IDL INTERFACE: ACCEPT A CONNECTION TO A SOCKET
 */
int
l4vfs_net_io_accept_component (CORBA_Object _dice_corba_obj,
                               object_handle_t s,
                               char addr[16],
                               l4vfs_socklen_t *addrlen,
                               l4vfs_socklen_t *actual_len,
                               short *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;

	*actual_len = *addrlen;
	err = accept(s,(struct sockaddr *) addr,actual_len);

	/* compute size of addr to send */
	if (*addrlen > *actual_len)
	        *addrlen = *actual_len;

	return err;
}

/** IDL INTERFACE: BIND
 */
int
l4vfs_net_io_bind_component (CORBA_Object _dice_corba_obj,
                             object_handle_t s,
                             const char addr[16],
                             int addrlen,
                             CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
    LOGd(_DEBUG,"fd: %d, addrlen: %d", s, addrlen);
	err = bind(s, (struct sockaddr *)addr, addrlen);
    LOGd(_DEBUG,"ret: %d", err);
	return err;
}

/** IDL INTERFACE: CONNECT
 */
int
l4vfs_net_io_connect_component (CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                const char addr[16],
                                int addrlen,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	LOGd(_DEBUG,"fd: %d, addrlen: %d", s, addrlen);
	err = connect(s, (struct sockaddr *)addr, addrlen);
	LOGd(_DEBUG, "ret: %d", err);
	return err;
}

/** IDL INTERFACE: GETSOCKNAME
 */
int
l4vfs_net_io_getsockname_component (CORBA_Object _dice_corba_obj,
                                    object_handle_t s,
                                    char name[4096],
                                    l4vfs_socklen_t *len,
                                    CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = getsockname(s, (struct sockaddr *)name, len);
	return err;
}

/** IDL INTERFACE: LISTEN
 */
l4_int32_t l4vfs_net_io_listen_component(CORBA_Object _dice_corba_obj,
                                         object_handle_t s,
                                         l4_int32_t backlog,
                                         CORBA_Server_Environment * _dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = listen(s, backlog);
	return err;
}

/** IDL INTERFACE: RECVFROM
 */
int
l4vfs_net_io_recvfrom_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t s,
                                 char **buf,
                                 l4vfs_size_t *len,
                                 int flags,
                                 char from[128],
                                 l4vfs_socklen_t *fromlen,
                                 l4vfs_socklen_t *actual_fromlen,
                                 short *_dice_reply,
                                 CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;

	*actual_fromlen = *fromlen;

	LOGd(_DEBUG, "fd (%d), len (%d), fromlen (%d)", s, *len, *fromlen);
	err = recvfrom(s, *buf, *len, flags, (struct sockaddr *)from, actual_fromlen);

	if (err > 0)
		*len = err;
	else
		*len = 0;

	/* compute size of from to send */
	if (*fromlen > *actual_fromlen)
		*fromlen = *actual_fromlen;

	LOGd(_DEBUG,"ret (%d), fromlen (%d)", err, *fromlen);
	return err;
}

/** IDL INTERFACE: RECV
 */
int
l4vfs_net_io_recv_component (CORBA_Object _dice_corba_obj,
                             object_handle_t s,
                             char **buf,
                             l4vfs_size_t *len,
                             int flags,
                             short *_dice_reply,
                             CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	LOGd(_DEBUG, "+recv (len=%d)", *len);
	err = recv(s, *buf, *len, flags);
	/* reply received or 0 bytes */
	if (err > 0)
		*len = err;
	else
		*len = 0;
	LOGd(_DEBUG, "++recv (len=%d)", *len);
	return err;
}

/** IDL INTERFACE: SEND
 */
int
l4vfs_net_io_send_component (CORBA_Object _dice_corba_obj,
                             object_handle_t s,
                             const char *msg,
                             int len,
                             int flags,
                             short *_dice_reply,
                             CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = send(s, (l4_int8_t *)msg, len, flags);
	return err;
}

/** IDL INTERFACE: SENDMSG
 */
int
l4vfs_net_io_sendmsg_component (CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                const char msg_name[8192],
                                int msg_namelen,
                                const char *msg_iov,
                                int msg_iovlen,
                                int msg_iov_size,
                                const char *msg_control,
                                int msg_controllen,
                                int msg_flags,
                                int flags,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int err, i;
	struct msghdr msg;
	void *offset;
	struct iovec *iov = (struct iovec *) msg_iov;

	memset(&msg, 0, sizeof(struct msghdr));

	 /* calculate offset to first iovec buffer */
	offset = (char *) msg_iov + sizeof(struct iovec) * msg_iovlen;

	for (i=0; i < msg_iovlen; i++)
	{
		iov->iov_base = offset;

		/* set new offset size */
		offset += iov->iov_len;

		iov++;
	}

	if (msg_namelen > 0)
		msg.msg_name = (void *) msg_name;
	msg.msg_namelen = msg_namelen;

	msg.msg_iov = (struct iovec *) msg_iov;
	msg.msg_iovlen = msg_iovlen;

	if (msg_controllen > 0)
		msg.msg_control = (void *) msg_control;
	msg.msg_controllen = msg_controllen;

	msg.msg_flags = msg_flags;

	err = sendmsg(s, &msg, flags);
	return err;
}

/** IDL INTERFACE: SENDTO
 */
int
l4vfs_net_io_sendto_component (CORBA_Object _dice_corba_obj,
                               object_handle_t s,
                               const char *msg,
                               int len,
                               int flags,
                               const char *to,
                               int tolen,
                               short *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
	int ret;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	LOGd(_DEBUG,"sendto(%d,%p,%d,%d,%p,%d) called",
	        s, msg, len, flags, to, tolen);
	ret = sendto(s, (l4_int8_t *)msg, len, flags, (struct sockaddr *)to, tolen);
	LOGd(_DEBUG, "back in component - sendto returned %d", ret);
	return ret;
}

/** IDL INTERFACE: SHUTDOWN
 */
int l4vfs_net_io_shutdown_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t s,
                                    l4_int32_t how, 
                                    CORBA_Server_Environment * _dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = shutdown(s, how);
	return err;
}


/** IDL INTERFACE: OPEN SOCKET
 */
l4_int32_t l4vfs_net_io_socket_component(CORBA_Object _dice_corba_obj,
                                         l4_int32_t domain,
                                         l4_int32_t type,
                                         l4_int32_t protocol,
                                         CORBA_Server_Environment * _dice_corba_env)
{
	int ret;
	LOGd(_DEBUG,"socket called: domain=%d, type=%d, protocol=%d", domain,
	        type, protocol);
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	ret = socket(domain, type, protocol);
	LOGd(_DEBUG,"back in component - got socket descriptor %d", ret);
	return ret;
}


/** IDL INTERFACE: SETSOCKOPT
 */
int
l4vfs_net_io_setsockopt_component (CORBA_Object _dice_corba_obj,
                                   object_handle_t s,
                                   int level,
                                   int optname,
                                   const char *optval,
                                   int optlen,
                                   CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = setsockopt(s,level,optname,optval,optlen);
	return err;
}

/** IDL INTERFACE: GETSOCKOPT
 */
int
l4vfs_net_io_getsockopt_component (CORBA_Object _dice_corba_obj,
                                   object_handle_t s,
                                   int level,
                                   int optname,
                                   char *optval,
                                   l4vfs_socklen_t *optlen,
				   l4vfs_socklen_t *actual_optlen,
                                   CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;

	*actual_optlen = *optlen;
	
	err = getsockopt(s,level,optname,optval,actual_optlen);
	
	/* compute size of addr to send */
	if (*optlen > *actual_optlen)
	        *optlen = *actual_optlen;

	return err;
}

/** IDL INTERFACE: GETPEERNAME
 */
int
l4vfs_net_io_getpeername_component (CORBA_Object _dice_corba_obj,
                                    object_handle_t s,
                                    char addr[120],
                                    l4vfs_socklen_t *addrlen,
                                    l4vfs_socklen_t *actual_len,
                                    CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;

	*actual_len = *addrlen;
	err = getpeername(s,(struct sockaddr *) addr,actual_len);
	LOGd(_DEBUG, "ip of peer: %s", inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));

	/* compute size of addr to send */
	if (*addrlen > *actual_len)
	        *addrlen = *actual_len;

	return err;
}

l4_int32_t
l4vfs_common_io_fcntl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                long *arg,
                                CORBA_Server_Environment *_dice_corba_env)
{
	l4_int32_t err;
	if (!client_owns_sid(CORBA_Object, fd))
		return -1;

	err=fcntl(fd, cmd, arg);

	LOGd(_DEBUG, "fcntl of FLIPS; returning err=%d cmd=%d", err, cmd);
	return err;
}
