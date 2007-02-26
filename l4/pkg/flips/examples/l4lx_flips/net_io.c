/**
 * \file   flips/examples/l4lx_flips/net_io.c
 * \brief  NET IO component implementation
 *
 * \date   02/03/2006
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*
 * The code herin is taken from flips/lib/server/libflips_servr.c amd reworked
 * to fit L4Linux 2.6's threading scheme for hybrid tasks. -- Christian
 */

#include "local.h"

/*** GENERAL INCLUDES ***/
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>

/*** IDL INCLUDES ***/
#include <flips-server.h>

/****************************/
/*** NET IO IDL INTERFACE ***/
/****************************/

/*** NET IO IDL: ACCEPT ***/
int
l4vfs_net_io_accept_component(CORBA_Object _dice_corba_obj,
                              object_handle_t s,
                              char addr[120],
                              int *addrlen,
                              int *actual_len,
                              short *_dice_reply,
                              CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	/* set buffer size */
	*actual_len = *addrlen;
	err = accept(s,(struct sockaddr *) addr,actual_len);

	/* compute size of addr to send and always send needed size in actual_len */
	if (*addrlen > *actual_len)
		*addrlen = *actual_len;

	return err;
}

/*** NET IO IDL: BIND ***/
int
l4vfs_net_io_bind_component(CORBA_Object _dice_corba_obj,
                            object_handle_t s,
                            const char addr[120],
                            int addrlen,
                            CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = bind(s, (struct sockaddr *)addr, addrlen);

	return err;
}

/*** NET IO IDL: CONNECT ***/
int
l4vfs_net_io_connect_component(CORBA_Object _dice_corba_obj,
                               object_handle_t s,
                               const char addr[120],
                               int addrlen,
                               short *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = connect(s, (struct sockaddr *)addr, addrlen);

	return err;
}

/*** NET IO IDL: GETSOCKNAME ***/
int
l4vfs_net_io_getsockname_component(CORBA_Object _dice_corba_obj,
                                   object_handle_t s,
                                   char name[4096],
                                   int *len,
                                   CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = getsockname(s, (struct sockaddr *)name, len);

	return err;
}

/*** NET IO IDL: LISTEN ***/
int
l4vfs_net_io_listen_component(CORBA_Object _dice_corba_obj,
                              object_handle_t s,
                              int backlog,
                              CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = listen(s, backlog);

	return err;
}

/*** NET IO IDL: RECVFROM ***/
int
l4vfs_net_io_recvfrom_component(CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                char **buf,
                                int *len,
                                int flags,
                                char from[128],
                                int *fromlen,
                                int *actual_fromlen,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	*actual_fromlen = *fromlen;

	err = recvfrom(s, *buf, *len, flags, (struct sockaddr *)from, actual_fromlen);

	if (err > 0) *len = err;
	else *len = 0;

	/* compute size of from to send */
	if (*fromlen > *actual_fromlen)
		*fromlen = *actual_fromlen;

	return err;
}

/*** NET IO IDL: RECV ***/
int
l4vfs_net_io_recv_component(CORBA_Object _dice_corba_obj,
                            object_handle_t s,
                            char **buf,
                            int *len,
                            int flags,
                            short *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = recv(s, *buf, *len, flags);

	/* reply received or 0 bytes */
	if (err > 0) *len = err;
	else *len = 0;

	return err;
}

/*** NET IO IDL: SEND ***/
int
l4vfs_net_io_send_component(CORBA_Object _dice_corba_obj,
                            object_handle_t s,
                            const char *msg,
                            int len,
                            int flags,
                            short *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = send(s, msg, len, flags);

	return err;
}

/*** NET IO IDL: SENDMSG ***/
int
l4vfs_net_io_sendmsg_component(CORBA_Object _dice_corba_obj,
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

	for (i=0; i < msg_iovlen; i++) {
		iov->iov_base = offset;

		/* set new offset size */
		offset += iov->iov_len;

		iov++;
	}

	if (msg_namelen > 0) msg.msg_name = (void *) msg_name;
	msg.msg_namelen = msg_namelen;

	msg.msg_iov = (struct iovec *) msg_iov;
	msg.msg_iovlen = msg_iovlen;

	if (msg_controllen > 0) msg.msg_control = (void *) msg_control;
	msg.msg_controllen = msg_controllen;

	msg.msg_flags = msg_flags;

	err = sendmsg(s, &msg, flags);

	return err;
}

/*** NET IO IDL: SENDTO ***/
int
l4vfs_net_io_sendto_component(CORBA_Object _dice_corba_obj,
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

	ret = sendto(s, (l4_int8_t *)msg, len, flags, (struct sockaddr *)to, tolen);

	return ret;
}

/*** NET IO IDL: SHUTDOWN ***/
int l4vfs_net_io_shutdown_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t s,
                                    l4_int32_t how, 
                                    CORBA_Server_Environment * _dice_corba_env)
{
	int err;

	err = shutdown(s, how);

	return err;
}


/*** NET IO IDL: OPEN SOCKET ***/
int
l4vfs_net_io_socket_component(CORBA_Object _dice_corba_obj,
                              int domain,
                              int type,
                              int protocol,
                              CORBA_Server_Environment * _dice_corba_env)
{
	int ret;

	ret = socket(domain, type, protocol);

	return ret;
}


/*** NET IO IDL: SETSOCKOPT ***/
int
l4vfs_net_io_setsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  const char *optval,
                                  int optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = setsockopt(s,level,optname,optval,optlen);

	return err;
}

/*** NET IO IDL: GETSOCKOPT ***/
int
l4vfs_net_io_getsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  char *optval,
                                  int *optlen,
                                  int *actual_optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	*actual_optlen = *optlen;
	err = getsockopt(s,level,optname,optval,actual_optlen);

	/* compute size of addr to send and always send needed size in actual_len */
	if (*optlen > *actual_optlen)
		*optlen = *actual_optlen;

	return err;
}

/*** NET IO IDL: GETPEERNAME ***/
int
l4vfs_net_io_getpeername_component(CORBA_Object _dice_corba_obj,
                              object_handle_t s,
                              char addr[120],
                              int *addrlen,
                              int *actual_len,
                              CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	/* set buffer size */
	*actual_len = *addrlen;
	err = getpeername(s,(struct sockaddr *) addr,actual_len);

	/* compute size of addr to send and always send needed size in actual_len */
	if (*addrlen > *actual_len)
		*addrlen = *actual_len;

	return err;
}
