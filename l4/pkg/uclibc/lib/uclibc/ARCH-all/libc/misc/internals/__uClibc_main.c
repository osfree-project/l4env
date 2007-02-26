#include <stddef.h>		/* for size_t */
#include <l4/sys/consts.h>	/* for L4_PAGESIZE */

/* Dummy implementations used by exit() */
void (*__app_fini)(void);
void (*__rtld_fini)(void);

const char *__progname;
char **__environ;
size_t __pagesize = L4_PAGESIZE;

