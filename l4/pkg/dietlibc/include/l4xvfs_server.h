#ifndef __L4XVFS_SERVER_H_
#define __L4XVFS_SERVER_H__

#include <sys/stat.h>
#include <sys/types.h>

int     open(const char *name, int flags);
int     close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, void *buf, size_t count);
off_t   lseek(int fd, off_t offset, int whence);
int     fstat(int fd, struct stat *buf);

#endif
