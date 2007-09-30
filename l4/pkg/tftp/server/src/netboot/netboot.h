#ifndef __NETBOOT_NETBOOT_H
#define __NETBOOT_NETBOOT_H

#define NETBUF_SIZE	0x8000
#define GUNZIP_SIZE	0x8000

int  netboot_init (unsigned bufaddr, unsigned gunzipaddr);
int  netboot_open (char *filename);
int  netboot_seek (int offset);
int  netboot_dir (char *dirname);
int  netboot_read (unsigned char *buf, int len);
void netboot_close (void);
void netboot_media_change (void);
void netboot_set_server (in_addr server_addr);
void netboot_show_drivers (void);

extern char *err_list[];

extern int filemax;
extern int filepos;

#define netboot_errnum		errnum
#define netboot_incomplete	incomplete
#define netboot_filemax		filemax
#define netboot_filepos		filepos

#endif

