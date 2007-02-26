#include <stddef.h>		/* for size_t */
#include <l4/sys/consts.h>	/* for L4_PAGESIZE */

/* Dummy implementations used by exit() */
void (*__app_fini)(void);
void (*__rtld_fini)(void);

const char *__progname;
attribute_hidden const char *__uclibc_progname = NULL;
#ifdef __UCLIBC_HAS___PROGNAME__
strong_alias (__uclibc_progname, __progname)
#endif

char **__environ;
size_t __pagesize = L4_PAGESIZE;

extern void __uClibc_fini(void);
libc_hidden_proto(__uClibc_fini)
void __uClibc_fini(void)
{
  /* empty since pkg/crtx does this for us */
}
libc_hidden_def(__uClibc_fini)
