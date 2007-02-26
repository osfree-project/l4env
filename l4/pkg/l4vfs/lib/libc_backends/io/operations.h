#ifndef _OPERATIONS_H_
#define _OPERATIONS_H_

// these should not be necesarry, as they are defined in libc header
// files
/*
int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
int read(int fd, void *buf, size_t count);
*/

void init_io(void);

#endif
