#ifndef __V_SCANF_BACKEND_H__
#define __V_SCANF_BACKEND_H__

#include <stddef.h>
#include <cdefs.h>
#include <stdarg.h>

typedef int put_func(int, void*);
typedef int get_func(void*);

struct scanf_ops {
  void *data;
  get_func *get;
  put_func *put;
};


__BEGIN_DECLS

int __v_scanf(struct scanf_ops* fn, const char *format, va_list arg_ptr);

__END_DECLS


#endif // __V_SCANF_BACKEND_H__
