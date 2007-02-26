#ifndef LIBLINUX_H
#define LIBLINUX_H

#include <linux/proc_fs.h>

/*** INITIALIZATION ***/
extern int liblinux_init(unsigned int vmem_size, unsigned int kmem_size,
                         int dhcp);

/*** UTIL: Get addr of L4IO info page ***/
void * liblinux_get_l4io_info(void);

/*** USER INTERFACE TO /proc ***/
extern int  liblinux_proc_init(void);
extern void liblinux_proc_ls(char *path);
extern int  liblinux_proc_read(const char *path, char *dst, int offset,
                               int dst_len);
extern int  liblinux_proc_write(const char *path, char *src, int src_len);

extern void liblinux_init_notify(void);

extern int liblinux_notify_request(int, int, l4_threadid_t *);
extern int liblinux_notify_clear(int, int, l4_threadid_t *);
extern int liblinux_select(int);
extern int liblinux_get_max_fd(void);

#endif

