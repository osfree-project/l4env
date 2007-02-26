#ifndef __UTIL_H_uniq_sdasdf
#define __UTIL_H_uniq_sdasdf

extern void fill_addr(struct sockaddr_un *addr, int *len, char *path);
extern int simple_select(int fd, int mode);
extern long elapsed_time(struct timeval *tv0, struct timeval *tv1);

#endif /* __UTIL_H_uniq_sdasdf */
