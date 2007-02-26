#ifndef _DLFCN_H
#define _DLFCN_H

#include <bits/dlfcn.h>

typedef struct
{
  __const char *dli_fname;	/* File name of defining object.  */
  void *dli_fbase;		/* Load address of that object.  */
  __const char *dli_sname;	/* Name of nearest symbol.  */
  void *dli_saddr;		/* Exact value of nearest symbol.  */
} Dl_info;

extern void *dlopen (const char *file, int mode);
extern int   dlclose (void *handle);
extern void *dlsym (void *handle, const char *name);
extern char *dlerror (void);
extern int   dladdr (const void *address, Dl_info *info);
extern int   dlinfo (void);

#endif
