#ifndef BASE_CRITICAL_H
#define BASE_CRITICAL_H

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void base_critical_enter(void);
void base_critical_leave(void);

EXTERN_C_END

#endif
