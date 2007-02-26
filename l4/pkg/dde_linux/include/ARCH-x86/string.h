#ifndef _STRING_H
#define _STRING_H

#include <l4/sys/compiler.h>

#undef offsetof		/* we still might have included <linux/stddef.h> */
#include <stddef.h>	/* for size_t */

#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

/* prototypes which _are_ available in the Linux kernel */

EXTERN_C_BEGIN

char*  strdup(const char *s);
char*  strcpy(char *dest, const char *src);
size_t strlen(const char *s);
char*  strncpy(char *dest, const char *src, size_t n);
int    strcmp(const char *s1, const char *s2);

int    memcmp(const void *s1, const void *s2, size_t n);
void*  memcpy(void *dest, const void *src, size_t n);
void*  memset(void *s, int c, size_t n);

EXTERN_C_END

#endif

/* prototypes which are _not_ available in the Linux kernel */

EXTERN_C_BEGIN

int    strcasecmp(const char *s1, const char *s2);

EXTERN_C_END

#endif
