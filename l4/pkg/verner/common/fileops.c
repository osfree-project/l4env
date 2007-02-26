/*
 * \brief   file I/O specific for VERNER's demuxer
 * \date    2004-03-17
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 *
 * This file implements fileops_* functions for file-like I/O
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* L4/DROPS includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>

/* verner */
#include "arch_types.h"
#include "arch_globals.h"
#include "arch_plugins.h"

/* local */
#include "fileops.h"

/* configuration */
#include "verner_config.h"

#if VDEMUXER_BUILD_EXT2FS
#include "io_ext2fs.h"
#endif
#if VDEMUXER_BUILD_GRUBFS
#include "io_grubfs.h"
#endif
#if VDEMUXER_BUILD_RTNETFS
#include "io_rtns.h"
#endif


/* prefered functions */
static int (*fops_open)  (const char *__name, int __mode, ...) = NULL;
static int (*fops_close) (int __fd) = NULL;
static unsigned long (*fops_read)  (int __fd, void *__buf, unsigned long __n) = NULL;
static unsigned long (*fops_write) (int __fd, void *__buf, unsigned long __n) = NULL;
static long (*fops_lseek) (int __fd, long __offset, int __whence) = NULL;

/* additional functions */
static int (*fops_ftruncate) (int __fd, unsigned long __offset) = NULL;
static int (*fops_fstat)     (int __fd, struct stat *buf) = NULL;


/*
 * implementations of fileops.h using the selected I/O-Plugins follows
 */


/*****************************************************************************/
/**
 * \brief Determines file-I/O type and sets up all function ptr all I/O 
 * functions from demux-plugins & demuxer-libraries (should) use.
 * 
 * Here we look into the submitted filename/URL and setting up the correct 
 * file-I/O-plugin.
 * Currently supported types are:
 *        grub://filename   - using grubfs
 *        ext2fs://filename - using ext2fs file provider
 * Check "make config" to build with support for them.
 */
/*****************************************************************************/
int fileops_open(const char *__name, int __mode, ...)
{
    char *filename;

    filename  = (char*) __name;

    /* check if filename has an valid URL:// string or default one */
#if VDEMUXER_BUILD_GRUBFS
   if((!strncasecmp("grub://",__name,7)) || (!strncasecmp(VDEMUXER_DEFAULT_URL,"grub",4)))
   {
      /* setup determined fileops  */
      fops_open  = io_grubfs_open;
      fops_close = io_grubfs_close;
      fops_read  = io_grubfs_read;
      fops_write = io_grubfs_write;
      fops_lseek = io_grubfs_lseek;
      fops_ftruncate = io_grubfs_ftruncate;
      fops_fstat = io_grubfs_fstat;
      /* cut URL://-descriptor */
      if(!strncasecmp("grub://",__name,7))
      	filename =  (char*) (__name + 7);
      else
	filename  = (char*) __name;
   }
#endif
#if VDEMUXER_BUILD_EXT2FS
   if((!strncasecmp("ext2fs://",__name,9)) || (!strncasecmp(VDEMUXER_DEFAULT_URL,"ext2fs",4)))
   {
      /* setup determined fileops  */
      fops_open  = io_ext2fs_open;
      fops_close = io_ext2fs_close;
      fops_read  = io_ext2fs_read;
      fops_write = io_ext2fs_write;
      fops_lseek = io_ext2fs_lseek;
      fops_ftruncate = io_ext2fs_ftruncate;
      fops_fstat = io_ext2fs_fstat;
      /* cut URL://-descriptor */
      if(!strncasecmp("ext2fs://",__name,9))
      	filename =  (char*) (__name + 9);
      else
	filename  = (char*) __name;
   }
#endif
#if VDEMUXER_BUILD_RTNETFS
   if((!strncasecmp("rtns://",__name,7)) || (!strncasecmp(VDEMUXER_DEFAULT_URL,"rtns",4)))
   {
      /* setup determined fileops  */
      fops_open  = io_rtns_open;
      fops_close = io_rtns_close;
      fops_read  = io_rtns_read;
      fops_write = io_rtns_write;
      fops_lseek = io_rtns_lseek;
      fops_ftruncate = io_rtns_ftruncate;
      fops_fstat = io_rtns_fstat;
      /* cut URL://-descriptor */
      if(!strncasecmp("rtns://",__name,7))
      	filename =  (char*) (__name + 7);
      else
	filename  = (char*) __name;
   }
#endif
#if VDEMUXER_BUILD_GRUBFS
      if(fops_open  == io_grubfs_open) LOGdL (DEBUG_FILEIO,"using grubfs file-I/O");
#endif
#if VDEMUXER_BUILD_EXT2FS
      if(fops_open  == io_ext2fs_open) LOGdL (DEBUG_FILEIO,"using ext2fs file-I/O");
#endif
#if VDEMUXER_BUILD_RTNETFS
      if(fops_open  == io_rtns_open) LOGdL (DEBUG_FILEIO,"using rtns file-I/O");
#endif

    /* now call open */
    LOGdL (DEBUG_FILEIO,"opening %s", filename);
    if(fops_open) 
      return fops_open(filename,__mode);
    else    
      LOG_Error("fileops_open not initialized. No I/O-Plugin found!");
    return -L4_ENOTSUPP;
}


/*
 * close file 
 */
int fileops_close(int __fd)
{
  LOGdL (DEBUG_FILEIO,"close fd=%i",__fd);
  if(fops_close) 
    return fops_close(__fd);
  else    
     KDEBUG("fileops_close not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}


/*
 * read from file 
 */
unsigned long fileops_read(int __fd, void *__buf, unsigned long __n)
{
  LOGdL (DEBUG_FILEIO,"read fd=%i",__fd);
  if(fops_read) 
    return fops_read(__fd,__buf,__n);
  else    
    KDEBUG("fileeops_read not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}


/*
 * write into file 
 */
unsigned long fileops_write(int __fd, void *__buf, unsigned long __n)
{
  LOGdL (DEBUG_FILEIO,"write fd=%i",__fd);
  if(fops_write) 
    return fops_write(__fd,__buf,__n);
  else    
    KDEBUG("fileops_write not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}


/*
 * seek within file 
 */
long fileops_lseek(int __fd, long __offset, int __whence)
{
  LOGdL (DEBUG_FILEIO,"lseek fd=%i",__fd);
  if(fops_lseek) 
    return fops_lseek(__fd,__offset,__whence);
  else    
    KDEBUG("fileops_lseek not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}

/* additional functions */
int fileops_ftruncate(int __fd, unsigned long __offset)
{
  LOGdL (DEBUG_FILEIO,"ftruncate fd=%i",__fd);
  if(fops_ftruncate) 
    return fops_ftruncate(__fd,__offset);
  else    
    KDEBUG("fileops_ftruncate not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}

int fileops_fstat(int __fd, struct stat *buf)
{
  LOGdL (DEBUG_FILEIO,"fstat fd=%i",__fd);
  if(fops_fstat) 
    return fops_fstat(__fd,buf);
  else    
    KDEBUG("fileops_fstat not initialized. Open was not called!");
  return -L4_ENOTSUPP;
}

