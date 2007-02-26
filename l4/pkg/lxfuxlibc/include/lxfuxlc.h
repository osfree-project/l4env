/*
 * Header files for lxfuxlc.c
 */
#ifndef __LXFUXLC_H__
#define __LXFUXLC_H__

/* -- sys/time.h -- */
struct lx_timeval {
  long tv_sec;
  long tv_usec;
};

struct lx_timezone {
  int  tz_minuteswest;
  int  tz_dsttime;
};


typedef struct {
  unsigned long fds_bits[(1024/(8 * sizeof(unsigned long)))];
} lx_fd_set;

/* -- sys/poll.h */

typedef unsigned long int lx_nfds_t;

struct lx_pollfd {
  int fd;
  short int events;
  short int revents;
};

#define LX_POLLIN	0x001
#define LX_POLLPRI	0x002
#define LX_POLLOUT	0x004
#define LX_POLLRDNORM	0x040
#define LX_POLLRDBAND	0x080
#define LX_POLLWRNORM	0x100
#define LX_POLLWRBAND	0x200
#define LX_POLLMSG	0x400

#define LX_POLLERR	0x008
#define LX_POLLHUP	0x010
#define LX_POLLNVAL	0x020

/* -- */

extern int lx_errno;

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef signed int lx_pid_t;

struct lx_stat {
	unsigned short st_dev;
	unsigned short __pad1;
	unsigned long st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned short st_rdev;
	unsigned short __pad2;
	unsigned long  st_size;
	unsigned long  st_blksize;
	unsigned long  st_blocks;
	unsigned long  st_atime;
	unsigned long  __unused1;
	unsigned long  st_mtime;
	unsigned long  __unused2;
	unsigned long  st_ctime;
	unsigned long  __unused3;
	unsigned long  __unused4;
	unsigned long  __unused5;
};

/*
 * fcntl defines
 */
#define LX_O_RDONLY		00
#define LX_O_WRONLY		01
#define LX_O_RDWR		02
#define LX_O_CREAT	      0100 /* not fcntl */
#define LX_O_EXCL	      0200 /* not fcntl */
#define LX_O_NOCTTY	      0400 /* not fcntl */
#define LX_O_TRUNC	     01000 /* not fcntl */
#define LX_O_APPEND	     02000
#define LX_O_NONBLOCK        04000
#define LX_O_NDELAY     O_NONBLOCK
#define LX_O_SYNC	    010000
#define LX_FASYNC	    020000 /* fcntl, for BSD compatibility */
#define LX_O_DIRECT	    040000 /* direct disk access hint */
#define LX_O_LARGEFILE     0100000
#define LX_O_DIRECTORY     0200000 /* must be a directory */
#define LX_O_NOFOLLOW      0400000 /* don't follow links */


/*
 * Syscall functions
 */

extern void lx_exit(int status) __attribute__ ((noreturn));
extern lx_pid_t lx_fork(void);
extern long lx_read(unsigned int fd, const char *buf, unsigned int count);
extern long lx_write(unsigned int fd, const char *buf, unsigned int count);
extern long lx_open(const char *filename, int flags, int mode);
extern long lx_close(unsigned int fd);
extern lx_pid_t lx_waitpid(lx_pid_t pid, int * wait_stat, int options);
extern unsigned long lx_lseek(unsigned int fd, unsigned long offset, unsigned int origin);
extern long lx_getpid(void);
extern int  lx_pipe(int filesdes[2]);
extern long lx_gettimeofday(struct lx_timeval *tv, struct lx_timezone *tz);
extern int  lx_stat(const char *filename, struct lx_stat *buf);
extern int  lx_fstat(int filedes, struct lx_stat *buf);
extern int  lx_lstat(const char *filename, struct lx_stat *buf);
extern int  lx_ipc(unsigned int call, int first, int second, int third, const void *ptr, long fifth);
extern int  lx_select(int n, lx_fd_set *readfds, lx_fd_set *writefds, lx_fd_set *exceptfds, struct lx_timeval *timeout);
extern int  lx_poll(struct lx_pollfd *fds, lx_nfds_t nfds, int timeout);

/*
 * Wrapper functions
 */

extern long lx_time(int *tloc);
extern unsigned int lx_sleep(unsigned int seconds);
extern unsigned int lx_msleep(unsigned int mseconds);
extern lx_pid_t lx_wait(int *wait_stat);

extern void *lx_shmat(int shmid, const void *shmaddr, int shmflg);

extern void lx_outchar(unsigned char c);
extern void lx_outdec32(unsigned int i);

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif


#define SHMAT		21

#endif /* __LXFUXLC_H__ */
