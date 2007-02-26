/*
  syscall.c -- override open(2)

  (c) Han-Wen Nienhuys <hanwen@cs.uu.nl> 1998
  (c) Jork Loeser <jork.loeser@inf.tu-dresden.de> 2002
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <syscall-list.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <unistd.h>
#include "gendep.h"

/*
  This breaks if we hook gendep onto another library that overrides open(2).
  (zlibc comes to mind)
 */
static int
real_open (const char *fn, int flags, int mode)
{
  return (syscall(SYS_open, (fn), (flags), (mode)));
}

int
__open (const char *fn, int flags, ...)
{
  int rv ;
  va_list p;
  va_start (p,flags);
    
  rv = real_open (fn, flags, va_arg (p, int));
  if (rv >=0)
    gendep__register_open (fn, flags);

  return rv;    
}

static int
real_unlink (const char *fn)
{
  return syscall(SYS_unlink, (fn));
}

int
__unlink (const char *fn)
{
  int rv ;
    
  rv = real_unlink (fn);
  if (rv >=0)
    gendep__register_unlink (fn);

  return rv;    
}



int open (const char *fn, int flags, ...) __attribute__ ((alias ("__open")));
int unlink(const char *fn) __attribute__ ((alias ("__unlink")));
