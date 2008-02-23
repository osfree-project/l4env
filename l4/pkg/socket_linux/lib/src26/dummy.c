#include <l4/log/l4log.h>

#include <linux/fs.h>
#include <linux/mm.h>

#ifdef SOCKET_LINUX_DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

/*** net/socket.c ***/

int move_addr_to_user(void *kaddr, int klen, void *uaddr, int *ulen);
int move_addr_to_user(void *kaddr, int klen, void *uaddr, int *ulen)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
    memcpy(uaddr, kaddr, klen);
    *ulen = klen;
    return 0;
}

int move_addr_to_kernel(void *uaddr, int ulen, void *kaddr);
int move_addr_to_kernel(void *uaddr, int ulen, void *kaddr)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
    memcpy(kaddr, uaddr, ulen);
    return 0;
}

/*** fs/fcntl.c (needed by socket.c) ***/

struct fasync_struct;

void __kill_fasync(struct fasync_struct *fa, int sig, int band)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
};

/*** fs/file_table.c (needed by core/scm.c) ***/

void fput(struct file *file)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
}

struct file *fget(unsigned int fd);
struct file *fget(unsigned int fd)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
    return (struct file *)0;
}

/*** fs/open.c (needed by core/scm.c) ***/

int get_unused_fd(void);
int get_unused_fd(void)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
    return -1;
}

struct file;

void fd_install(unsigned int fd, struct file *file);
void fd_install(unsigned int fd, struct file *file)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
    return;
}

/*** fs/select.c (needed by core/datagram.c) ***
 *
 * This is used if someone polls or selects sockets. While this is a nice
 * feature we do not implement it for now but it's a TODO.
 */

#include <linux/poll.h>

void __pollwait(struct file *filp, wait_queue_head_t * wait_address,
                poll_table * p)
{
    static int cnt = 0;
    LOGd_Enter(_DEBUG,"(%i)", ++cnt);
}

