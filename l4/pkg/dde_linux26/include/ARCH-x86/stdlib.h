#ifndef _STDLIB_H
#define _STDLIB_H

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

EXTERN_C_BEGIN

void              exit(int status);
void*             malloc(l4_size_t size);
void              free(void *ptr);
int               atoi(const char *nptr);
long              atol(const char *nptr);
long int          strtol(const char *nptr, char **endptr, int base);
unsigned long int strtoul(const char *nptr, char **endptr, int base);

EXTERN_C_END

#endif
