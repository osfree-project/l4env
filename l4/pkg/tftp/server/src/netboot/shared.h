#ifndef __SHARED_H
#define __SHARED_H

#define FSYS_BUFLEN	NETBUF_SIZE
#define NETWORK_DRIVE	0x20
#define RAW_ADDR(x)	(x)
#define MAXINT		0x7FFFFFFF
#define SECTOR_SIZE	0x200

extern int buf_drive;
extern int fsys_type;
extern unsigned long current_drive;
extern struct multiboot_info mbi;
extern int compressed_file;
extern char config_file[];

extern unsigned long saved_drive;
extern unsigned long saved_partition;
extern unsigned long current_partition;

extern void (*disk_read_func) (int, int, int);
extern void (*disk_read_hook) (int, int, int);

int nul_terminate (char *str);
int safe_parse_maxint (char **str_ptr, int *myint_ptr);

int gunzip_test_header (void);
int gunzip_read (char *buf, int len);

char *set_device (char *device);
int open_device (void);

#endif /* __SHARED_H */
