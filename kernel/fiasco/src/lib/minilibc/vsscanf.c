#include <stdarg.h>
#include <stdlib.h>
#include "vscanf_backend.h"

struct str_data {
  unsigned char* str;
};

static int sgetc(struct str_data* sd) {
  register unsigned int ret = *(sd->str++);
  return (ret)?(int)ret:-1;
}

static int sputc(int c, struct str_data* sd) {
  return (*(--sd->str)==c)?c:-1;
}

int vsscanf(const char* str, const char* format, va_list arg_ptr)
{
  struct str_data  fdat = { (unsigned char*)str };
  struct scanf_ops farg = { (void*)&fdat, (get_func*)&sgetc, (put_func*)&sputc };
  return __v_scanf(&farg,format,arg_ptr);
}
