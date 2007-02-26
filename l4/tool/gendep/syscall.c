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
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <unistd.h>
/* Assume GNU as target platform. We need this in dlfcn.h */
#define __USE_GNU
#include <dlfcn.h>
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

typedef FILE* (*fopen_type)(const char*, const char*);

static FILE* real_fopen(const char*path, const char*mode){
  static fopen_type f_fopen;
  
  if(f_fopen==0){
    f_fopen = (fopen_type)dlsym(RTLD_NEXT, "fopen");
    if(!f_fopen){
      fprintf(stderr, "gendep: Cannot resolve fopen()\n");
      errno=ENOENT;
      return 0;
    }
  }
  return f_fopen(path, mode);
}

FILE* __fopen(const char*path, const char*mode){
  FILE *f;
  int binmode;

  f = real_fopen(path, mode);
  if(f){
    if(strchr(mode, 'w') || strchr(mode, 'a')){
      binmode=O_WRONLY;
    } else {
      binmode=O_RDONLY;
    }
    gendep__register_open(path, binmode);
  }
  return f;
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
FILE *fopen (const char *path, const char *mode) __attribute__ ((alias ("__fopen")));
