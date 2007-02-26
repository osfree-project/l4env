#include <linux/vmalloc.h>
#include <linux/net.h>
#include <net/scm.h>
#include <net/sock.h>
#include <asm/uaccess.h>

#include <l4/socket_linux/socket_linux.h>

extern void sk_init(void);              /* from core/sock.c */

#define MAX_SOCK_ADDR   128
#define MAX_SOCKETS     128

/* the local data structure */
typedef struct socket_lnx_data {
    struct socket *sock;
    void *priv_data;
} socket_lnx_data_t ;

static int sock_alloc_counter = 0;
static struct net_proto_family *net_families[NPROTO];
static socket_lnx_data_t sockets[MAX_SOCKETS];

/** UTILITY: MAP SOCKET FILE DESCRIPTOR TO SOCKET STRUCTURE
 */
struct socket *sockfd_lookup(int sd, int *err)
{
	if ((sd < 0) || (sd >= MAX_SOCKETS)) {
		if (err)
			*err = -EINVAL;
		return NULL;
	}
	if (err)
		*err = 0;
	return sockets[sd].sock;
}


/** UTILITY: ASSIGN A FILE DESCRIPTOR TO A SOCKET STRUCTURE
 *
 * Returns new socket file descriptor or a negative error code if
 * there is no free socket file descriptor available.
 */
int sock_map_fd(struct socket *sock)
{
	int i;

	/* search for free socket handler id */
	for (i = 0; i < MAX_SOCKETS; i++) {
		if (!sockets[i].sock)
			break;
	}

	if (i >= MAX_SOCKETS)
		return -ENOMEM;

	sockets[i].sock = sock;
	return i;
}


/** UTILITY: MOVE SOCKADDR FROM CALLER */
int move_addr_from(void *uaddr, int ulen, void *kaddr)
{
	if ((ulen < 0) || (ulen > MAX_SOCK_ADDR))
		return -EINVAL;
	if (ulen == 0)
		return 0;
	memcpy(kaddr, uaddr, ulen);
	return 0;
}

/** UTILITY: MOVE SOCKADDR TO CALLER */
int move_addr_to(void *kaddr, int klen, void *uaddr, int *ulen)
{
	int len = *ulen;

	if (len > klen)
		len = klen;
	if ((len < 0) || (len > MAX_SOCK_ADDR))
		return -EINVAL;
	if (len) {
		memcpy(uaddr, kaddr, len);
	}
	/*
	 *	"fromlen shall refer to the value before truncation.."
	 *			1003.1g
	 */
	*ulen = klen;
	return 0;
}

/** ALLOCATE AND INIT A SOCKET STRUCTURE
 *
 * Allocate and init a new socket structure. Returns pointer to freshly
 * allocated struct or NULL if an allocation error occured.
 */
struct socket *sock_alloc(void)
{
	struct socket *sock = (struct socket *) vmalloc(sizeof(struct socket));

	if (!sock)
		return NULL;

	init_waitqueue_head(&sock->wait);
	sock->inode = NULL;
	sock->fasync_list = NULL;
	sock->state = SS_UNCONNECTED;
	sock->flags = 0;
	sock->ops = NULL;
	sock->sk = NULL;
	sock->file = NULL;

	sock_alloc_counter++;
	return sock;
}


/** FREE SOCKET STRUCTURE
 */
void sock_free(struct socket *sock)
{
	vfree(sock);
	sock_alloc_counter--;
}


/** CREATE NEW SOCKET
 *
 * Creates a new socket of the specified family and protocol and returns 
 * its socket structure in res. The only supported family is AF_INET. Only
 * SOCK_STREAM and SOCK_DGRAM types are supported.
 */
int sock_create(int family, int type, int protocol, struct socket **res)
{
	int resval = 0;
	struct socket *sock;

	if ((family < 0) || (family >= NPROTO))
		return -EAFNOSUPPORT;

	if (!(sock = sock_alloc())) {
		printk(KERN_WARNING "socket: no more sockets\n");
		return -ENFILE;
	}

	sock->type = type;

	if (net_families[family] && net_families[family]->create) {
		if ((resval = net_families[family]->create(sock, protocol)) < 0) {
			sock_release(sock);
			return resval;
		}
	} else
		return -EAFNOSUPPORT;

	*res = sock;
	return resval;
}


/** CLOSE A SOCKET
 *
 * Close a socket and free its socket structure.
 */
void sock_release(struct socket *sock)
{
	if ((sock->ops) && (sock->ops->release))
		sock->ops->release(sock);
	sock_alloc_counter--;
	vfree(sock);
}


/** REGISTER ADDRESS FAMILY
 *
 * Since only one address family (AF_INET) is supported this function
 * only set a function pointer.
 */
int sock_register(struct net_proto_family *fam)
{
	if (!fam)
		return -EINVAL;
	if ((fam->family < 0) || (fam->family >= NPROTO))
		return -EINVAL;
	net_families[fam->family] = fam;
	return 0;
}


/** UNREGISTER ADDRESS FAMILY
 *
 * dummy implementation
 */
int sock_unregister(int family)
{
	return 0;
}


/** WAKE ASYNC
 *
 * Maybe god or a brave Linux kernel hacker knows for what the heck
 * this is good for...
 */
int sock_wake_async(struct socket *sock, int how, int band)
{
	if (!sock || !sock->fasync_list)
		return -1;
	switch (how) {
	case 1:
		if (test_bit(SOCK_ASYNC_WAITDATA, &sock->flags))
			break;
		goto call_kill;
	case 2:
		if (!test_and_clear_bit(SOCK_ASYNC_NOSPACE, &sock->flags))
			break;
		/* fall through */
	case 0:
	  call_kill:
		__kill_fasync(sock->fasync_list, SIGIO, band);
		break;
	case 3:
		__kill_fasync(sock->fasync_list, SIGURG, band);
	}
	return 0;
}


/** SOCKET GET INFO
 *
 * returns some statistics
 * This function is only called by net/ipv4/proc.c
 */
int socket_get_info(char *buffer, char **start, off_t offset, int length)
{
	int len;
	len = sprintf(buffer, "sockets: used %d\n", sock_alloc_counter);
	if (offset >= len) {
		*start = buffer;
		return 0;
	}
	*start = buffer + offset;
	len -= offset;
	if (len > length)
		len = length;
	if (len < 0)
		len = 0;
	return len;
}


/** WRAP IT (NEEDED BY CORE/SOCK.C - THANKS!)
 */
int sock_sendmsg(struct socket *sock, struct msghdr *msg, int size)
{
	return sock->ops->sendmsg(sock, msg, size, NULL);
}

/* SOCKET INTERFACE
 *
 * XXX The original socket() et al. return -1 on error. One has to use errno
 * to obtain any error value.
 */

//int socketpair (int d, int type, int protocol, int *sv[2]) {}
//int getpeername (int s, struct sockaddr *name, int *namelen) {}

/** SOCKET INTERFACE: SENDMSG
 */
int sendmsg(int fd, const struct msghdr *msg, int flags)
{
	struct socket *sock;
	char address[MAX_SOCK_ADDR];
	struct iovec iovstack[UIO_FASTIOV], *iov = iovstack;
	/* 20 is size of ipv6_pktinfo */
	unsigned char ctl[sizeof(struct cmsghdr) + 20];
	unsigned char *ctl_buf = ctl;
	struct msghdr msg_sys;
	int err, ctl_len, iov_size, total_len;

	err = -EFAULT;
	if (!msg)
		goto out;

	memcpy(&msg_sys,msg,sizeof(struct msghdr));

	sock = sockfd_lookup(fd, &err);
	if (!sock)
		goto out;

	/* do not move before msg_sys is valid */
	err = -EMSGSIZE;
	if (msg_sys.msg_iovlen > UIO_MAXIOV)
		goto out;

	/* Check whether to allocate the iovec area*/
	err = -ENOMEM;
	iov_size = msg_sys.msg_iovlen * sizeof(struct iovec);
	if (msg_sys.msg_iovlen > UIO_FASTIOV) {
		iov = (struct iovec *) sock_kmalloc(sock->sk, iov_size, GFP_KERNEL);
		if (!iov)
			return err;
	}

	err = verify_iovec(&msg_sys, iov, address, VERIFY_READ);
	if (err < 0)
		goto out_freeiov;
	total_len = err;

	err = -ENOBUFS;

	if (msg_sys.msg_controllen > INT_MAX)
		goto out_freeiov;
	ctl_len = msg_sys.msg_controllen;
	if (ctl_len)
	{
		if (ctl_len > sizeof(ctl))
		{
			ctl_buf = sock_kmalloc(sock->sk, ctl_len, GFP_KERNEL);
			if (ctl_buf == NULL)
				goto out_freeiov;
		}
		memcpy(ctl_buf, msg_sys.msg_control, ctl_len);
		msg_sys.msg_control = ctl_buf;
	}
	msg_sys.msg_flags = flags;

	if (sock->file)
	{
		if (sock->file->f_flags & O_NONBLOCK)
		{
			msg_sys.msg_flags |= MSG_DONTWAIT;
		}
	}

	err = sock_sendmsg(sock, &msg_sys, total_len);

	if (ctl_buf != ctl)
		sock_kfree_s(sock->sk, ctl_buf, ctl_len);
out_freeiov:
	if (iov != iovstack)
		sock_kfree_s(sock->sk, iov, iov_size);
out:
	return err;

}

/** SOCKET INTERFACE: SETSOCKOPT
 */
int setsockopt (int s, int level, int optname, const void *optval, int optlen)
{
	int err;
	struct socket *sock;

	if (optlen < 0)
		return -EINVAL;

	sock = sockfd_lookup(s, &err);

	if (!sock)
		return err;
        
	if (level == SOL_SOCKET)
		err=sock_setsockopt(sock,level,optname,(char *) optval, 
	                            optlen);
        else
	        err=sock->ops->setsockopt(sock, level, optname, 
					  (char *) optval, optlen);

	return err;
}

/** SOCKET INTERFACE: GETSOCKOPT
 */
int getsockopt (int s, int level, int optname, const void *optval, int * optlen)
{
	int err;
	struct socket *sock;

	if (optlen < 0)
		return -EINVAL;

	sock = sockfd_lookup(s, &err);

	if (!sock)
		return err;
        
	if (level == SOL_SOCKET)
		err=sock_getsockopt(sock,level,optname,(char *) optval, 
	                            optlen);
        else
	        err=sock->ops->getsockopt(sock, level, optname, 
					  (char *) optval, optlen);

	return err;
}

/** SOCKET INTERFACE: ACCEPT
 */
int accept(int sd, struct sockaddr *uaddr, int *uaddrlen)
{

	struct socket *sock, *newsock;
	int err, len;
	char address[MAX_SOCK_ADDR];

	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	err = -EMFILE;
	if (!(newsock = sock_alloc()))
		return err;

	newsock->type = sock->type;
	newsock->ops = sock->ops;

	err = sock->ops->accept(sock, newsock, O_RDWR);
	if (err < 0) {
		sock_release(newsock);
		return err;
	}

	if (uaddr) {
		if (newsock->ops->
		      getname(newsock, (struct sockaddr *)address, &len, 2) < 0) {
			sock_release(newsock);
			return -ECONNABORTED;
		}
		if ((err = move_addr_to(address, len, uaddr, uaddrlen)))
			return err;
	}

	if ((err = sock_map_fd(newsock)) < 0)
		sock_release(newsock);
	return err;
}


/** SOCKET INTERFACE: BIND
 */
int bind(int sd, struct sockaddr *my_addr, int my_addrlen)
{
	struct socket *sock;
	char address[MAX_SOCK_ADDR];
	int err;

	if (!my_addr)
		return -EINVAL;
	if (!(sock = sockfd_lookup(sd, &err)))
		return err;
	if ((err = move_addr_from(my_addr, my_addrlen, address)))
		return err;
	err = sock->ops->bind(sock, (struct sockaddr *)address, my_addrlen);
	return err;
}


/** SOCKET INTERFACE: CONNECT
 */
int connect(int sd, struct sockaddr *serv_addr, int addrlen)
{
	char address[MAX_SOCK_ADDR];
	int err;
	/* XXX Maybe sockfd_lookup() has strange semantics ... */
	struct socket *sock = sockfd_lookup(sd, &err);

	if (!sock)
		return err;
	if ((err = move_addr_from(serv_addr, addrlen, address)))
		return err;
	err = sock->ops->connect(sock, (struct sockaddr *)address, addrlen, O_RDWR);
	return err;
}


/** SOCKET INTERFACE: GETSOCKNAME
 */
int getsockname(int s, struct sockaddr *name, int *namelen)
{
	struct socket *sock;
	char address[MAX_SOCK_ADDR];
	int len, err;

	sock = sockfd_lookup(s, &err);
	if (!sock)
		return err;
	err = sock->ops->getname(sock, (struct sockaddr *)address, &len, 0);
	if (err)
		return err;
	return move_addr_to(address, len, name, namelen);
}

int sysctl_somaxconn = SOMAXCONN;

/** SOCKET INTERFACE: LISTEN
 */
int listen(int sd, int backlog)
{
	struct socket *sock;
	int err;

	if ((sock = sockfd_lookup(sd, &err)) != NULL) {
		if ((unsigned)backlog > SOMAXCONN)
			backlog = SOMAXCONN;
		err = sock->ops->listen(sock, backlog);
	}
	return err;
}


/** SOCKET INTERFACE: RECVMSG
 */
int recvmsg(int sd, struct msghdr *msg, int flags)
{
	struct socket *sock;
	struct iovec iovstack[UIO_FASTIOV];
	struct iovec *iov = iovstack;
	struct msghdr msg_sys;
	unsigned long cmsg_ptr;
	int err, iov_size, total_len;
	char addr[MAX_SOCK_ADDR];
	struct sockaddr *uaddr;
	struct scm_cookie scm;
	int *uaddr_len;

	err = -EFAULT;
	memcpy(&msg_sys, msg, sizeof(struct msghdr));
	memset(&scm, 0, sizeof(scm));

	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	err = -EINVAL;
	if (msg_sys.msg_iovlen > UIO_MAXIOV)
		return err;

	/* Check whether to allocate the iovec area */
	err = -ENOMEM;
	iov_size = msg_sys.msg_iovlen * sizeof(struct iovec);
	if (msg_sys.msg_iovlen > UIO_FASTIOV) {
		iov = (struct iovec *) sock_kmalloc(sock->sk, iov_size, GFP_KERNEL);
		if (!iov)
			return err;
	}

	uaddr = msg_sys.msg_name;
	uaddr_len = &msg->msg_namelen;
	err = verify_iovec(&msg_sys, iov, addr, VERIFY_WRITE);
	if (err < 0)
		goto out_freeiov;
	total_len = err;

	cmsg_ptr = (unsigned long)msg_sys.msg_control;
	msg_sys.msg_flags = 0;

	flags |= MSG_DONTWAIT;

	err = sock->ops->recvmsg(sock, &msg_sys, total_len, flags, &scm);
	if (err < 0)
		goto out_freeiov;

	if (uaddr && msg_sys.msg_namelen) {
		err = move_addr_to_user(addr, msg_sys.msg_namelen, uaddr, uaddr_len);
		if (err < 0)
			goto out_freeiov;
	}
	msg->msg_flags = msg_sys.msg_flags;
	msg->msg_controllen = (unsigned long)(msg_sys.msg_control - cmsg_ptr);

  out_freeiov:
	if (iov != iovstack)
		sock_kfree_s(sock->sk, iov, iov_size);
	return err;
}


/** SOCKET INTERFACE: RECVFROM
 */
int recvfrom(int sd, void *ubuf, size_t size, int flags,
             struct sockaddr *addr, int *addr_len)
{

	struct socket *sock;
	struct iovec iov;
	struct msghdr msg;
	char address[MAX_SOCK_ADDR];
	struct scm_cookie scm;
	int err, err2;

	if (!(sock = sockfd_lookup(sd, &err)))
		return err;

	memset(&scm, 0, sizeof(scm));

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_namelen = MAX_SOCK_ADDR;
	msg.msg_iovlen = 1;
	msg.msg_iov = &iov;
	msg.msg_name = address;

	iov.iov_len = size;
	iov.iov_base = ubuf;

	err = sock->ops->recvmsg(sock, &msg, size, flags, &scm);

	if (err >= 0 && addr != NULL && msg.msg_namelen) {
		err2 = move_addr_to(address, msg.msg_namelen, addr, addr_len);
		if (err2 < 0) return err;
	}
	return err;
}


/** SOCKET INTERFACE: RECV
 */
int recv(int sd, void *buf, size_t len, int flags)
{
	return recvfrom(sd, buf, len, flags, NULL, NULL);
}


/** SOCKET INTERFACE: SENDTO
 *
 * Send a message from a socket.
 */
int
sendto(int sd, void *msgbuff, size_t len, int flags,
       struct sockaddr *to, int to_len)
{
	struct socket *sock;
	char address[MAX_SOCK_ADDR];
	int err = 0;
	struct msghdr msg;
	struct scm_cookie scm;
	struct iovec iov;

	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	iov.iov_base = msgbuff;
	iov.iov_len = len;
	msg.msg_name = NULL;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_namelen = 0;
	msg.msg_controllen = 0;

	memset(&scm, 0, sizeof(scm));

	if (to) {
		err = move_addr_from(to, to_len, address);
		if (err < 0) return err;
		msg.msg_name = address;
		msg.msg_namelen = to_len;
	}
	msg.msg_flags = flags;
	err = sock->ops->sendmsg(sock, &msg, len, &scm);
	return err;
}


/** SOCKET INTERFACE: SEND
 */
int send(int sd, void *msgbuff, size_t len, int flags)
{
	return sendto(sd, msgbuff, len, flags, NULL, 0);
}


/** SOCKET INTERFACE: SHUTDOWN
 */
int shutdown(int sd, int how)
{
	int retval;
	struct socket *sock;

	if ((sock = sockfd_lookup(sd, &retval))) {
		if ((sock->ops) && (sock->ops->shutdown)) {
			retval = sock->ops->shutdown(sock, how);
		}
	}
	return retval;
}


/** SOCKET INTERFACE: SOCKET
 *
 * Create new socket communication channel of specified family and type.
 * Returns socket descriptor.
 */
int socket(int family, int type, int protocol)
{
	int retval;
	struct socket *sock;

	retval = sock_create(family, type, protocol, &sock);
	if (retval < 0)
		return retval;

	retval = sock_map_fd(sock);
	if (retval < 0)
		sock_release(sock);
	return retval;
}


/** SOCKET INTERFACE: IOCTL
 *
 * Do an ioctl() on socket.
 * Returns error code if socket descriptor is invalid.
 */
int socket_ioctl(int sd, unsigned int cmd, void *arg)
{
	int err;
	struct socket *sock = sockfd_lookup(sd, &err);

	if (!sock)
		return err;

	err = sock->ops->ioctl(sock, cmd, (unsigned long)arg);
	return err;
}

/** SOCKET INTERFACE: READ
 */
int socket_read(int sd, char *ubuf, size_t size)
{
	struct socket *sock;
	struct iovec iov;
	struct scm_cookie scm;
	struct msghdr msg;
	int flags = 0, err;

	if (size == 0)              /* Match SYS5 behaviour */
		return 0;

	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	iov.iov_base = ubuf;
	iov.iov_len = size;
//        flags = !(file->f_flags & O_NONBLOCK) ? 0 : MSG_DONTWAIT;

	memset(&scm, 0, sizeof(scm));

	return sock->ops->recvmsg(sock, &msg, size, flags, &scm);
}

/** SOCKET INTERFACE: WRITE
 */
int socket_write(int sd, const char *ubuf, size_t size)
{
	struct socket *sock;
	struct msghdr msg;
	struct scm_cookie scm;
	struct iovec iov;
	int err;

	if (size == 0)              /* Match SYS5 behaviour */
		return 0;

	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
//msg.msg_flags=!(file->f_flags & O_NONBLOCK) ? 0 : MSG_DONTWAIT;
	if (sock->type == SOCK_SEQPACKET)
		msg.msg_flags |= MSG_EOR;
	iov.iov_base = (void *)ubuf;
	iov.iov_len = size;

	memset(&scm, 0, sizeof(scm));

	return sock->ops->sendmsg(sock, &msg, size, &scm);
}

/** SOCKET INTERFACE: CLOSE
 *
 * Closes socket and frees its associated socket structure and socket
 * descriptor. Returns error code if socket descriptor is invalid.
 */
int socket_close(int sd)
{
	int err;
	struct socket *sock = sockfd_lookup(sd, &err);

	if (err < 0)
		return err;
	if (sock)
		sock_release(sock);
	sockets[sd].sock = NULL;
	sockets[sd].priv_data = NULL;
	return 0;
}


/** INITIALISATION OF SOCKET SUBSYSTEM
 *
 * This function must be called once during the startup of the system.
 */
int libsocket_linux_init()
{
	int i;
	for (i = 0; i < NPROTO; i++)
		net_families[i] = NULL;

	sk_init();
	skb_init();

	return 0;
}

/** SET PRIVATE DATA OF SOCKET
 *
 * This funtion must be called to set private data of a socket.
 */
int socket_set_private_data(int sd, void *private_data)
{
	int err;
	struct socket *sock;
	sock = sockfd_lookup(sd, &err);
	if (!sock)
		return err;

	sockets[sd].priv_data = private_data;

	return 0;
}

/** GET PRIVATE DATA OF SOCKET
 *
 * This function returns private data of a socket.
 */
void * socket_get_private_data(int sd)
{
	return sockets[sd].priv_data;
}

int getpeername(int s, struct sockaddr *name, int *namelen)
{
	int err;
	struct socket *sock;

	sock = sockfd_lookup(s, &err);

	if (!sock)
		return err;
        
	err = sock->ops->getname(sock, name, namelen, 1);

	return err;
}
       
