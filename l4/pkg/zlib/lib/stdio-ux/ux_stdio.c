/*!
 * \file   zlib/lib/stdio-ux/ux_stdio.c
 * \brief  This file maps the l4zlib_* calls to lx_* calls
 *
 * \date   05/17/2004
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*
 * fprintf is just defined because it's needed, it's not implemented
 *  (will cause a page fault)!
 */

#include <l4/lxfuxlibc/lxfuxlc.h>

#define DUP(x) x;x

DUP(LX_FILE *l4zlib_fopen(const char *name, const char *mode))
{
  return lx_fopen(name, mode);
}

DUP(LX_FILE *l4zlib_fdopen(int fd, const char *mode))
{
  return lx_fdopen(fd, mode);
}

DUP(lx_size_t l4zlib_fread(void *ptr, lx_size_t size, lx_size_t nmemb, LX_FILE *f))
{
  return lx_fread(ptr, size, nmemb, f);
}

DUP(lx_size_t l4zlib_fwrite(const void *ptr, lx_size_t size, lx_size_t nmemb, LX_FILE *f))
{
  return lx_fwrite(ptr, size, nmemb, f);
}

DUP(int l4zlib_fclose(LX_FILE *f))
{
  return lx_fclose(f);
}

DUP(long l4zlib_ftell(LX_FILE *f))
{
  return lx_ftell(f);
}

DUP(int l4zlib_fseek(LX_FILE *f, long offset, int whence))
{
  return lx_fseek(f, offset, whence);
}

DUP(void l4zlib_rewind(LX_FILE *f))
{
  lx_rewind(f);
}

DUP(int l4zlib_fflush(LX_FILE *f))
{
  return lx_fflush(f);
}

DUP(int l4zlib_fputc(int c, LX_FILE *f))
{
  return lx_fputc(c, f);
}

DUP(int l4zlib_fprintf(LX_FILE *f, const char *format, ...))
{
  *(int *)0xafe = 1234;
  return 0;
}
