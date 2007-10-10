/**
 * \file   l4vfs/lib/backends/socket_io/socket.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/connection.h>
#include <l4/l4vfs/net_io.h>
#include <l4/l4vfs/file-table.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/names/libnames.h>

static l4_threadid_t ip_server = L4_INVALID_ID;
static l4_threadid_t pf_key_server = L4_INVALID_ID;
static l4_threadid_t pf_local_server = L4_INVALID_ID;
static l4_threadid_t ip_server_thread = L4_INVALID_ID;
static l4_threadid_t pf_key_server_thread = L4_INVALID_ID;
static l4_threadid_t pf_local_server_thread = L4_INVALID_ID;

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

#define IPSTACK "FLIPS"
#define PFKEY_SERV "PF_KEY"
#define PFLOCAL_SERV "PF_LOCAL"

static int get_socket_server_thread(int domain, l4_threadid_t *server_thread);
static int get_socket_server_thread(int domain, l4_threadid_t *server_thread)
{
    l4_threadid_t *server;
    l4_threadid_t *thread;
    const char    *server_name;
    const char    *names_name;

    /*
     * currently we support only these domains:
     * PF_INET, PF_NETLINK, PF_KEY, PF_LOCAL
     *
     * We have three socket servers, which are currently hardwired.
     * It will be generalized in next versions.
     */
    if (domain == PF_INET || domain == PF_NETLINK)
    {
	server = &ip_server;
	thread = &ip_server_thread;
	server_name = "IP Stack";
	names_name  = IPSTACK;
    }
    else if (domain == PF_KEY)
    {
	server = &pf_key_server;
	thread = &pf_key_server_thread;
	server_name = "PF_KEY Server";
	names_name  = PFKEY_SERV;
    }
    else if (domain == PF_LOCAL)
    {
	server = &pf_local_server;
	thread = &pf_local_server_thread;
	server_name = "PF_LOCAL Server";
	names_name  = PFLOCAL_SERV;
    }
    else
        return -EPFNOSUPPORT;


    /* get thread IDs */
    if (l4_is_invalid_id(*server))
    {
	if (names_waitfor_name(names_name, server, 10000) == 0)
	{
	    LOG("%s is not registered at names!\n", server_name);
	    return -ENETDOWN;
	}
	if (l4_is_invalid_id(*thread))
	{
	    *thread = l4vfs_init_connection(*server);
            if (l4_is_invalid_id(*thread))
            {
                LOG_Error("Couldn't build up connection with %s!\n",
			  server_name); 
	        return -ENETDOWN;
	    }
	}
    }

    *server_thread = *thread;
    return 0;
}

int socket(int domain, int type, int protocol)
{
    int ret, local_fd;
    file_desc_t fd_s;
    l4_threadid_t server_thread;
    object_handle_t object_handle;

    local_fd = ft_get_next_free_entry();
    if (local_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", local_fd);

    // get thread ID of socket server specified by 'domain'
    ret = get_socket_server_thread(domain, &server_thread);
    if (ret < 0)
    {
	errno = -ret;
	return -1;
    }

    object_handle = l4vfs_socket(server_thread, domain, type, protocol);

    // check for error
    if (object_handle < 0)
    {
        errno = -object_handle;
        return -1;
    }

    fd_s.server_id = server_thread;
    fd_s.object_handle = object_handle;
    fd_s.object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    fd_s.object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;

    LOGd(_DEBUG, "object_handle '%d'", object_handle);

    ft_fill_entry(local_fd, fd_s);

    return local_fd;
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
    int ret;
    int local_fd[2];
    file_desc_t fd_s[2];
    l4_threadid_t server_thread;
    object_handle_t object_handle[2];

    // get thread ID of socket server specified by 'domain'
    ret = get_socket_server_thread(domain, &server_thread);
    if (ret < 0)
    {
	errno = -ret;
	return -1;
    }

    local_fd[0] = ft_get_next_free_entry();
    if (local_fd[0] == -1)
    {
        errno = EMFILE;
        return -1;
    }
    // we need ft_fill_entry() here to make sure that the 2nd
    // ft_get_next_free_entry() returns another entry (we use
    // dummy values until l4vfs_socketpair() returns the real
    // object_handles)
    fd_s[0].server_id     = server_thread;
    fd_s[0].object_handle = 1234;
    ft_fill_entry(local_fd[0], fd_s[0]);

    local_fd[1] = ft_get_next_free_entry();
    if (local_fd[1] == -1)
    {
	ft_free_entry(local_fd[0]);
        errno = EMFILE;
        return -1;
    }
    fd_s[1].server_id = server_thread;

    LOGd(_DEBUG, "local fds '%d', '%d'", local_fd[0], local_fd[1]);

    ret = l4vfs_socketpair(server_thread, domain, type, protocol,
			   &object_handle[0],  &object_handle[1]);

    // check for error
    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    LOGd(_DEBUG, "object_handles '%d', '%d'", object_handle[0], object_handle[1]);

    fd_s[0].object_handle       = object_handle[0];
    fd_s[0].object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    fd_s[0].object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    ft_fill_entry(local_fd[0], fd_s[0]); // this time for real

    fd_s[1].object_handle       = object_handle[1]; 
    fd_s[1].object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    fd_s[1].object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    ft_fill_entry(local_fd[1], fd_s[1]);

    // finally set out parameters
    sv[0] = local_fd[0];
    sv[1] = local_fd[1];

    return ret;
}

int bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)
{
    int ret;
    file_desc_t file_desc;

    if (! my_addr)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", sockfd);
    if (! ft_is_open(sockfd))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(sockfd);

    ret = l4vfs_bind(file_desc.server_id, file_desc.object_handle,
                    (struct sockaddr *) my_addr, addrlen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int listen(int s, int backlog)
{
    int ret;
    file_desc_t file_desc;

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_listen(file_desc.server_id, file_desc.object_handle,
                       backlog);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int local_fd;
    file_desc_t file_desc, fd_s;
    object_handle_t accept_obj_handle = -1;
    socklen_t addrlen_tmp = 0;

    if (! addr)
    {
        addrlen = &addrlen_tmp;
    }
    else
    {
        if (! addrlen)
        {
            errno = EFAULT;
            return -1;
        }
    }

    local_fd = ft_get_next_free_entry();
    if (local_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    accept_obj_handle = l4vfs_accept(file_desc.server_id,
                                     file_desc.object_handle,
                                     addr, addrlen);

    if (accept_obj_handle < 0)
    {
        errno = -accept_obj_handle;
        return -1;
    }

    fd_s.server_id           = file_desc.server_id;
    fd_s.object_handle       = accept_obj_handle;
    fd_s.object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    fd_s.object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;

    LOGd(_DEBUG, "object_handle for accepted socket '%d'", accept_obj_handle);

    ft_fill_entry(local_fd, fd_s);

    return local_fd;
}

int connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen)
{
    int ret;
    file_desc_t file_desc;

    if (! serv_addr)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_connect(file_desc.server_id, file_desc.object_handle,
                        (struct sockaddr *) serv_addr, addrlen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}


ssize_t recv(int s, void *buf, size_t len, int flags)
{
    int ret;
    file_desc_t file_desc;
    char *b = buf;

    if (len < 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (! buf)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_recv(file_desc.server_id, file_desc.object_handle,
                     &b, len, flags);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return ret;
}

ssize_t recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from,
                 socklen_t *fromlen)
{
    int ret;
    file_desc_t file_desc;
    char *b = buf;

    if (len < 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (! buf)
    {
        errno = EFAULT;
        return -1;
    }

    if (from && ! fromlen)
    {
        errno = EINVAL;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_recvfrom(file_desc.server_id, file_desc.object_handle,
                         &b, len, flags, (struct sockaddr *) from, fromlen);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return ret;
}

ssize_t send(int s, const void *msg, size_t len, int flags)
{
    int ret;
    file_desc_t file_desc;

    if (! msg)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_send(file_desc.server_id, file_desc.object_handle,
                     msg, len, flags);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return ret;
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
    int ret,i;
    size_t msg_iov_size = 0;
    void *offset;
    char *msg_iov;
    struct iovec *io_vec, *curr_msg_iov;
    file_desc_t file_desc;

    if (! msg)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    io_vec = msg->msg_iov;

   /* we want to create the following buffer structure
    * in msg_iov:
    *        ------------
    * ------|------------|-------------------------------
    * iovec_1 | iovec_2 | iovec_1_buffer | iovec_2_buffer
    * ----------------|-------------------|--------------
    *                  -------------------
    */

    /* calculate malloc size of msg_iov buffer */
    for (i=0;i<msg->msg_iovlen;i++)
    {
        msg_iov_size += io_vec[i].iov_len + sizeof(struct iovec);
    }

    LOGd(_DEBUG,"msg_iov_size: %zd",msg_iov_size);

    msg_iov = malloc (msg_iov_size);

    if (! msg_iov)
    {
        errno = ENOMEM;
        return -1;
    }

    curr_msg_iov = (struct iovec *) msg_iov;

    /* calculate offset to first iovec buffer */
    offset = msg_iov + sizeof(struct iovec) * msg->msg_iovlen;

    for (i=0;i<msg->msg_iovlen;i++)
    {
        curr_msg_iov->iov_len = io_vec[i].iov_len;

        /* set new iov_base address */
        curr_msg_iov->iov_base = offset;

        memcpy(curr_msg_iov->iov_base, io_vec[i].iov_base,
               io_vec[i].iov_len);

        /* set new offset size */
        offset += io_vec[i].iov_len;

        curr_msg_iov++;
    }

    ret = l4vfs_sendmsg(file_desc.server_id, file_desc.object_handle,
                        msg->msg_name, msg->msg_namelen,
                        msg_iov, msg->msg_iovlen, msg_iov_size,
                        msg->msg_control, msg->msg_controllen,
                        msg->msg_flags, flags);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return ret;
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen)
{
    int ret;
    file_desc_t file_desc;

    if (! msg || ! to)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_sendto(file_desc.server_id, file_desc.object_handle,
                       msg, len, flags, (struct sockaddr *) to, tolen);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return ret;
}

int shutdown(int s, int how)
{
    int ret;
    file_desc_t file_desc;

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_shutdown(file_desc.server_id, file_desc.object_handle, how);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    // other cases SHUT_RD, SHUT_WR have to be considered in IP Stack server
    // only, because local fd_descr doesn't have knowledge about it
    if (how == SHUT_RDWR)
    {
        ft_free_entry(s); // true means closing local file descriptor
    }

    return 0;
}

int getsockname(int  s , struct sockaddr * name , socklen_t * namelen)
{
    int ret;
    file_desc_t file_desc;

    if (! name || ! namelen)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_getsockname(file_desc.server_id, file_desc.object_handle,
                            name, namelen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int setsockopt(int s, int level, int optname, const void *optval,
               socklen_t optlen)
{
    int ret;
    file_desc_t file_desc;

    if (! optval)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_setsockopt(file_desc.server_id, file_desc.object_handle,
                           level, optname, optval, optlen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int getsockopt(int s, int level, int optname, void *optval,
               socklen_t * optlen)
{
    int ret;
    file_desc_t file_desc;

    if (! optval)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_getsockopt(file_desc.server_id, file_desc.object_handle,
                           level, optname, optval, optlen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;
    file_desc_t file_desc;

    if (! name || ! namelen)
    {
        errno = EFAULT;
        return -1;
    }

    LOGd(_DEBUG, "local fd '%d'", s);
    if (! ft_is_open(s))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(s);

    ret = l4vfs_getpeername(file_desc.server_id,
                            file_desc.object_handle,
                            name, namelen);

    if (ret != 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}
