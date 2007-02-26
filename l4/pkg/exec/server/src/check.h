#ifndef __L4_EXEC_SERVER_CHECK_H
#define __L4_EXEC_SERVER_CHECK_H

int check(int error, const char *format, ...)
  __attribute__ ((format (printf, 2, 3)));

#endif

