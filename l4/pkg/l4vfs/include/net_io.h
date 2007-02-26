/**
 * \file   l4vfs/include/net_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_NET_IO_H_
#define __L4VFS_INCLUDE_NET_IO_H_

#include <l4/sys/compiler.h>
#include <l4/l4vfs/types.h>
#include <l4/l4vfs/common_io.h>
#include <l4/l4vfs/net_io-client.h>

EXTERN_C_BEGIN

object_handle_t l4vfs_socket(l4_threadid_t server,
                             int domain,
                             int type,
                             int protocol);

int l4vfs_socketpair(l4_threadid_t server,
                     int domain,
                     int type,
                     int protocol,
                     object_handle_t *fd0,
                     object_handle_t *fd1);

int             l4vfs_accept(l4_threadid_t server,
                             object_handle_t fd,
                             struct sockaddr *addr,
                             socklen_t *addrlen);

int             l4vfs_bind(l4_threadid_t server,
                           object_handle_t fd,
                           struct sockaddr *addr,
                           socklen_t addrlen);

int             l4vfs_connect(l4_threadid_t server,
                              object_handle_t fd,
                              struct sockaddr *addr,
                              socklen_t addrlen);

int             l4vfs_listen(l4_threadid_t server,
                             object_handle_t fd,
                             int backlog);

int             l4vfs_recvfrom(l4_threadid_t server,
                               object_handle_t fd,
                               char **buf,
                               size_t len,
                               int flags,
                               struct sockaddr *from,
                               socklen_t *fromlen);

int             l4vfs_recv(l4_threadid_t server,
                           object_handle_t fd,
                           char **buf,
                           size_t len,
                           int flags);

int             l4vfs_send(l4_threadid_t server,
                           object_handle_t fd,
                           const void *msg,
                           size_t len,
                           int flags);

int         l4vfs_sendmsg(l4_threadid_t server,
                          object_handle_t fd,
                          const void *msg_name,
                          socklen_t msg_namelen,
                          const void *msg_iov,
                          size_t msg_iovlen,
                          size_t msg_iov_size,
                          const void *msg_control,
                          socklen_t msg_controllen,
                          int msg_flags,
                          int flags);

int             l4vfs_sendto(l4_threadid_t server,
                             object_handle_t fd,
                             const void *msg,
                             size_t len,
                             int flags,
                             struct sockaddr *to,
                             socklen_t tolen);

int             l4vfs_shutdown(l4_threadid_t server,
                               object_handle_t fd,
                               int how);

int             l4vfs_getsockname(l4_threadid_t server,
                                  object_handle_t fd,
                                  struct sockaddr * name,
                                  socklen_t * namelen);

int             l4vfs_setsockopt(l4_threadid_t server,
                                 object_handle_t fd,
                                 int level,
                                 int optname,
                                 const void *optval,
                                 socklen_t optlen);

int             l4vfs_getsockopt(l4_threadid_t server,
                                 object_handle_t fd,
                                 int level,
                                 int optname,
                                 const void *optval,
                                 socklen_t *optlen);

int             l4vfs_getpeername(l4_threadid_t server,
                             object_handle_t fd,
                             struct sockaddr *addr,
                             socklen_t *addrlen);

EXTERN_C_END

#endif
