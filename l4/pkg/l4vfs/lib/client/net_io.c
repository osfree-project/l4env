/**
 * \file   l4vfs/lib/client/net_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
/*** GENERAL INCLUDES ***/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/types.h>
#include <l4/l4vfs/net_io.h>
#include <l4/l4vfs/net_io-client.h>

object_handle_t l4vfs_socket(l4_threadid_t server,
                             int domain,
                             int type,
                             int protocol)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_socket_call(&server,
                                    domain,
                                    type,
                                    protocol,
                                    &_dice_corba_env);
}

int l4vfs_socketpair(l4_threadid_t server,
                     int domain,
                     int type,
                     int protocol,
                     object_handle_t *fd0,
                     object_handle_t *fd1)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_socketpair_call(&server,
                                        domain,
                                        type,
                                        protocol,
                                        fd0,
                                        fd1,
                                        &_dice_corba_env);
}

int l4vfs_accept(l4_threadid_t server,
                 object_handle_t fd,
                 struct sockaddr *addr,
                 socklen_t *addrlen)
{
    int buflen;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (addrlen)
    {
        buflen = *addrlen;
    }
    else
    {
        buflen = 0;
    }

    return l4vfs_net_io_accept_call(&server,
                                    fd,
                                    (char *) addr,
                                    &buflen,
                                    addrlen,
                                    &_dice_corba_env);

}

int l4vfs_bind(l4_threadid_t server,
               object_handle_t fd,
               struct sockaddr *addr,
               socklen_t addrlen)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_bind_call(&server,
                                  fd,
                                  (char *) addr,
                                  addrlen,
                                  &_dice_corba_env);
}

int l4vfs_connect(l4_threadid_t server,
                  object_handle_t fd,
                  struct sockaddr *addr,
                  socklen_t addrlen)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_connect_call(&server,
                                     fd,
                                     (char *) addr,
                                     addrlen,
                                     &_dice_corba_env);

}

int l4vfs_listen(l4_threadid_t server,
                 object_handle_t fd,
                 int backlog)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_listen_call(&server,
                                    fd,
                                    backlog,
                                    &_dice_corba_env);
}

int l4vfs_recvfrom(l4_threadid_t server,
                   object_handle_t fd,
                   l4_int8_t **buf,
                   size_t len,
                   int flags,
                   struct sockaddr *from,
                   socklen_t *fromlen)
{
    int buflen;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (fromlen)
    {
        buflen = *fromlen;
    }
    else
    {
        buflen = 0;
    }

    return l4vfs_net_io_recvfrom_call(&server,
                                      fd,
                                      buf,
                                      &len,
                                      flags,
                                      (char *) from,
                                      &buflen,
                                      fromlen,
                                      &_dice_corba_env);

}

int l4vfs_recv(l4_threadid_t server,
               object_handle_t fd,
               l4_int8_t **buf,
               size_t len,
               int flags)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_recv_call(&server,
                                  fd,
                                  buf,
                                  &len,
                                  flags,
                                  &_dice_corba_env);
}

int l4vfs_send(l4_threadid_t server,
               object_handle_t fd,
               const void *msg,
               size_t len,
               int flags)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_send_call(&server,
                                  fd,
                                  (char *) msg,
                                  len,
                                  flags,
                                  &_dice_corba_env);
}

int l4vfs_sendmsg(l4_threadid_t server,
                  object_handle_t fd,
                  const void *msg_name,
                  socklen_t msg_namelen,
                  const void *msg_iov,
                  size_t msg_iovlen,
                  size_t msg_iov_size,
                  const void *msg_control,
                  socklen_t msg_controllen,
                  int msg_flags,
                  int flags)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_sendmsg_call(&server,
                                     fd,
                                     msg_name,
                                     msg_namelen,
                                     msg_iov,
                                     msg_iovlen,
                                     msg_iov_size,
                                     msg_control,
                                     msg_controllen,
                                     msg_flags,
                                     flags,
                                     &_dice_corba_env);
}
int l4vfs_sendto(l4_threadid_t server,
                 object_handle_t fd,
                 const void *msg,
                 size_t len,
                 int flags,
                 struct sockaddr *to,
                 socklen_t tolen)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_sendto_call(&server,
                                    fd,
                                    (char *) msg,
                                    len,
                                    flags,
                                    (char *) to,
                                    tolen,
                                    &_dice_corba_env);
}

int l4vfs_shutdown(l4_threadid_t server,
                   object_handle_t fd,
                   int how)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_shutdown_call(&server,
                                      fd,
                                      how,
                                      &_dice_corba_env);
}

int l4vfs_getsockname(l4_threadid_t server,
                      object_handle_t fd,
                      struct sockaddr *name,
                      socklen_t *name_len)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_getsockname_call(&server,
                                         fd,
                                         (char *) name,
                                         name_len,
                                         &_dice_corba_env);
}

int l4vfs_setsockopt(l4_threadid_t server,
                     object_handle_t fd,
                     int level,
                     int optname,
                     const void *optval,
                     socklen_t optlen)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
   _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_net_io_setsockopt_call(&server,
                                        fd,
                                        level,
                                        optname,
                                        (char *) optval,
                                        optlen,
                                        &_dice_corba_env);
}
