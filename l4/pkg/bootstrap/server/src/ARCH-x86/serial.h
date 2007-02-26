#ifndef SERIAL_H
#define SERIAL_H

void com_cons_putchar(int ch);
int  com_cons_try_getchar(void);
void com_cons_init(int com_port);

#endif

