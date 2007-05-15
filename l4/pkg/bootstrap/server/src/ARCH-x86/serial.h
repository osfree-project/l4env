#ifndef SERIAL_H
#define SERIAL_H

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void com_cons_putchar(int ch);
int  com_cons_try_getchar(void);
void com_cons_init(int com_port);

EXTERN_C_END

#endif

