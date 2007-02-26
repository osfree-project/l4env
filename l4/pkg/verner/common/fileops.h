#ifndef _FILEOPS_H_
#define _FILEOPS_H_

/*
 * Operations for File-I/O used by many libs
 * all functions are prefixed by fileops to avoid conflicts with stdlib/io
 *
 * These functions are implemented in vdemuxer/vmuxer and using io-libs
 * for grubfs, ext2fs, ...
 */
#include <sys/stat.h>

/* prefered functions */
int fileops_open(const char *__name, int __mode, ...);
int fileops_close(int __fd);
unsigned long fileops_read(int __fd, void *__buf, unsigned long __n);
unsigned long fileops_write(int __fd, void *__buf, unsigned long __n);
long fileops_lseek(int __fd, long __offset, int __whence);

/* additional functions */
int fileops_ftruncate(int __fd, unsigned long __offset);
int fileops_fstat(int __fd, struct stat *buf);

#endif
