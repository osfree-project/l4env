/*
 * Some very minor system calls for Linux.
 */

#include <lxfuxlc.h>

/* unistd.h stuff */
#define __NR_exit		1
#define __NR_fork		2
#define __NR_read		3
#define __NR_write		4
#define __NR_open		5
#define __NR_close		6
#define __NR_waitpid		7
#define __NR_lseek		19
#define __NR_getpid		20
#define __NR_pipe		42
#define __NR_gettimeofday	78
#define __NR_stat		106
#define __NR_lstat		107
#define __NR_fstat		108
#define __NR_ipc		117
#define __NR_select		142 /* new select */
#define __NR_poll		168

int lx_errno;



/* Some stuff pilfered from linux/include/asm-i386/unistd.h */

/* user-visible error numbers are in the range -1 to -124, see asm/errno.h */
#define __lx_syscall_return(type, res)					\
do {									\
  if ((unsigned long)(res) >= (unsigned long)(-125)) {			\
    lx_errno = -(res);							\
    res = -1;								\
  }									\
  return (type) (res);							\
} while (0)

#define __lx_syscall0(type,name)					\
type lx_##name(void)							\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      : "=a" (__res)							\
      : "0" (__NR_##name)						\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

#define __lx_syscall1(type,name,type1,arg1)				\
type lx_##name(type1 arg1)						\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      : "=a" (__res)							\
      : "0" (__NR_##name),						\
        "b" ((long)(arg1))						\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

#define __lx_syscall1_e(type,name,type1,arg1)				\
type lx_##name(type1 arg1)						\
{									\
loop:									\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      :									\
      : "a" (__NR_##name),						\
        "b" ((long)(arg1))						\
      : "memory");							\
  goto loop;								\
}

#define __lx_syscall2(type,name,type1,arg1,type2,arg2)			\
type lx_##name(type1 arg1, type2 arg2)					\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      : "=a" (__res)							\
      : "0" (__NR_##name),						\
        "b" ((long)(arg1)), "c" ((long)(arg2))				\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

#define __lx_syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)	\
type lx_##name(type1 arg1, type2 arg2, type3 arg3)			\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      : "=a" (__res)							\
      : "0" (__NR_##name),						\
        "b" ((long)(arg1)), "c" ((long)(arg2)), "d" ((long)(arg3))	\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

#define __lx_syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)	\
type lx_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)	\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "int	$0x80\n"						\
      : "=a" (__res)							\
      : "0" (__NR_##name),						\
        "b" ((long)(arg1)), "c" ((long)(arg2)), "d" ((long)(arg3)),	\
	"S" ((long)(arg4)), "D" ((long)(arg5))				\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

#define __lx_syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)	\
type lx_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6)	\
{									\
  long __res;								\
  __asm__ __volatile__ (						\
      "push %%ebp ; movl %%eax,%%ebp ; movl %1,%%eax ; int $0x80 ; pop %%ebp"	\
      : "=a" (__res)							\
      : "i" (__NR_##name),						\
        "b" ((long)(arg1)), "c" ((long)(arg2)), "d" ((long)(arg3)),	\
	"S" ((long)(arg4)), "D" ((long)(arg5)), "0" ((long)(arg6))	\
      : "memory");							\
  __lx_syscall_return(type, __res);					\
}

__lx_syscall1_e(void, exit, int, status)
__lx_syscall0(lx_pid_t, fork)
__lx_syscall3(long, read,  unsigned int, fd, const char *, buf, unsigned int, count)
__lx_syscall3(long, write, unsigned int, fd, const char *, buf, unsigned int, count)
__lx_syscall3(long, open, const char *, filename, int, flags, int, mode)
__lx_syscall1(long, close, unsigned int, fd)
__lx_syscall3(lx_pid_t, waitpid, lx_pid_t, pid, int *, wait_stat, int, options)
__lx_syscall3(unsigned long, lseek, unsigned int, fd, unsigned long, offset, unsigned int, origin)
__lx_syscall0(long, getpid)
__lx_syscall1(int, pipe, int *, filedes)
__lx_syscall2(long, gettimeofday, struct lx_timeval *, tv, struct lx_timezone *, tz)
__lx_syscall2(int, stat, const char *, filename, struct lx_stat *, buf)
__lx_syscall2(int, fstat, int, filedes, struct lx_stat *, buf)
__lx_syscall2(int, lstat, const char *, filename, struct lx_stat *, buf)
__lx_syscall6(int, ipc, unsigned int, call, int, first, int, second, int, third, const void *, ptr, long, fifth)
__lx_syscall5(int, select, int, n, lx_fd_set *, readfds, lx_fd_set *, writefds, lx_fd_set *, exceptfds, struct lx_timeval *, timeout)
__lx_syscall3(int, poll, struct lx_pollfd *, fds, lx_nfds_t, nfds, int, timeout)


/* ========================================================================
 *                   pure wrapper stuff w/o syscalls
 * ========================================================================
 */

/* time could also be done through a syscall */
long lx_time(int *tloc)
{
  struct lx_timeval tv;
  if (lx_gettimeofday(&tv, NULL) == 0) {
    if (tloc)
      *tloc = tv.tv_sec;
    return tv.tv_sec;
  }
  return -1;
}

unsigned int lx_sleep(unsigned int seconds)
{
  struct lx_timeval tv;

  tv.tv_sec  = seconds;
  tv.tv_usec = 0;
  if (lx_select(1, NULL, NULL, NULL, &tv) < 0)
    return 0;

  return tv.tv_sec;
}

unsigned int lx_msleep(unsigned int mseconds)
{
  struct lx_timeval tv;

  tv.tv_sec  = 0;
  tv.tv_usec = mseconds * 1000;
  if (lx_select(1, NULL, NULL, NULL, &tv) < 0)
    return 0;

  return tv.tv_sec;
}

lx_pid_t lx_wait(int *wait_stat)
{
  return lx_waitpid(-1, wait_stat, 0);
}

void *lx_shmat(int shmid, const void *shmaddr, int shmflg)
{
  void *raddr;
  void *result = (void *)lx_ipc(SHMAT, shmid, shmflg, (int)&raddr, shmaddr, 0);
  if ((unsigned long)result <= -(unsigned long)8196)
    result = raddr;
  return result;

}

/* ------------------------------------------------------------------ */

void lx_outchar(unsigned char c)
{
  lx_write(1, &c, 1);
}

void lx_outdec32(unsigned int i)
{
  char xx[11]; /* int is 32bit -> up to 10 figues... */
  int count = 10;

  xx[10] = 0;
  do {
    count--;
    xx[count] = '0' + i % 10;
    i /= 10;
  } while (i);
  lx_write(1, &xx[count], 10 - count); 
}
