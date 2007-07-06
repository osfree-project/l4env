#ifndef _DL_SYSCALL_H
#define _DL_SYSCALL_H

/* Protections are chosen from these bits, OR'd together.  The
   implementation does not necessarily support PROT_EXEC or PROT_WRITE
   without PROT_READ.  The only guarantees are that no writing will be
   allowed without PROT_WRITE and no access will be allowed for PROT_NONE. */

#define	PROT_NONE	 0x00	/* No access.  */
#define	PROT_READ	 0x04	/* Pages can be read.  */
#define	PROT_WRITE	 0x02	/* Pages can be written.  */
#define	PROT_EXEC	 0x01	/* Pages can be executed.  */

#define MAP_FIXED	 0x10   /* Interpret addr exactly.  */

/* Sharing types (must choose one and only one of these).  */
#define MAP_ANONYMOUS	 0x0002 /* Allocated from anonymous virtual memory.  */
#define MAP_COPY	 0x0020	/* Virtual copy of region at mapping time.  */
#define	MAP_SHARED	 0x0010	/* Share changes.  */
#define	MAP_PRIVATE	 0x0000	/* Changes private; copy pages on write.  */

#define O_RDONLY	 0x0000
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT		   0100

#define _dl_MAX_ERRNO	 4096

#define _dl_mmap_check_error(__res)     \
          (((long)__res) < 0 && ((long)__res) >= -_dl_MAX_ERRNO)

void _dl_exit(int code);
void _dl_close(int fd);
void* _dl_mmap(void *start, unsigned size, int prot, int flags,
               int fd, unsigned offset);
int  _dl_munmap(void *start, unsigned size);
int  _dl_mprotect(const void *addr, unsigned len, int prot);
unsigned short _dl_getuid(void);
unsigned short _dl_geteuid(void);
unsigned short _dl_getgid(void);
unsigned short _dl_getegid(void);
int _dl_open(const char *name, int flags, unsigned mode);
unsigned _dl_read(int fd, void *buf, unsigned count);
int _dl_readlink(const char *path, char *buf, unsigned size);
int _dl_write(int fd, const char *str, unsigned size);
static inline int _dl_getpid(void)
{
  return 0;
}

#endif
