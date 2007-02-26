#ifndef FOO_H
#define FOO_H

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

unsigned foo_add(unsigned a, unsigned b);
void     foo_show(void);

EXTERN_C_END

#endif
