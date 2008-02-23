#include <linux/net.h>
#include <linux/aio.h>
#include <linux/sched.h> //send_sig
#include <l4/log/l4log.h>
#include <linux/random.h> //srandom32, random32
#include <linux/bootmem.h> // nr_all_pages
#include <linux/mm.h> //si_meminfo
#include <linux/vmalloc.h> //__vmalloc


//used in tcp.c (2.6.20.19)
//number of all pages available
//at least 128 pages are used, if this number is to small
unsigned long nr_all_pages = 256;

// used by net/ipv4/inetpeer.c (2.6.20.19)
// inetpeer evaluates only totalram
void si_meminfo(struct sysinfo *val)
{
	val->totalram = 256;
	val->sharedram = 0;
	val->freeram = 0;
	val->bufferram = 0;
	val->totalhigh = 0;
	val->freehigh = 0;
	val->mem_unit = PAGE_SIZE;
}

// used by net/core/request_sock.c (2.6.20.19)
// together with __vmalloc
unsigned long long __PAGE_KERNEL = 0;

// used by net/core/request_sock.c (2.6.20.19)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
{
  LOG("not implemented");
  return 0;
}

inline int kernel_sendmsg(struct socket *sock, struct msghdr *msg,
		    struct kvec *vec, size_t num, size_t len)
{
  LOG("not implemented");
  return -1;
}

inline ssize_t fastcall wait_on_sync_kiocb(struct kiocb *iocb)
{
  LOG("not implemented");
  return -1;
}

void srandom32(u32 seed)
{
  LOG("not implemented");
} 

u32 random32(void)
{
  LOG("not implemented");
  return 0;
}

int send_sig(int a, struct task_struct * b, int c)
{
  LOG("not implemented");
  return -1;
}

int send_sigurg(struct fown_struct *fown)
{
  LOG("not implemented");
  return 0;
}
