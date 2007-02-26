#ifndef __NETBOOT_NETBOOT_H
#define __NETBOOT_NETBOOT_H

#define NETBUF_SIZE	0x8000
#define GUNZIP_SIZE	0x8000

extern int  netboot_init (unsigned int bufaddr, unsigned int gunzipaddr);
extern int  netboot_open (char *filename);
extern int  netboot_seek (int offset);
extern int  netboot_dir (char *dirname);
extern int  netboot_read (char *buf, int len);
extern void netboot_close (void);
extern void netboot_media_change (void);

typedef enum
{
  ERR_NONE = 0,
  ERR_BAD_FILENAME,
  ERR_BAD_FILETYPE,
  ERR_BAD_GZIP_DATA,
  ERR_BAD_GZIP_HEADER,
  ERR_BAD_PART_TABLE,
  ERR_BAD_VERSION,
  ERR_BELOW_1MB,
  ERR_BOOT_COMMAND,
  ERR_BOOT_FAILURE,
  ERR_BOOT_FEATURES,
  ERR_DEV_FORMAT,
  ERR_DEV_VALUES,
  ERR_EXEC_FORMAT,
  ERR_FILELENGTH,
  ERR_FILE_NOT_FOUND,
  ERR_FSYS_CORRUPT,
  ERR_FSYS_MOUNT,
  ERR_GEOM,
  ERR_NEED_LX_KERNEL,
  ERR_NEED_MB_KERNEL,
  ERR_NO_DISK,
  ERR_NO_PART,
  ERR_NUMBER_PARSING,
  ERR_OUTSIDE_PART,
  ERR_READ,
  ERR_SYMLINK_LOOP,
  ERR_UNRECOGNIZED,
  ERR_WONT_FIT,
  ERR_WRITE,
  ERR_BAD_ARGUMENT,
  ERR_UNALIGNED,
  ERR_PRIVILEGED,

  MAX_ERR_NUM
} netboot_error_t;

extern char *err_list[];

extern netboot_error_t errnum;
extern int incomplete;
extern int filemax;
extern int filepos;

#define netboot_errnum		errnum
#define netboot_incomplete	incomplete
#define netboot_filemax		filemax
#define netboot_filepos		filepos

#endif

