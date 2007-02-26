#include <libc_backend.h>

#include "boot_direct_cons.h"

int __libc_backend_outs( const char *s, size_t len )
{
  size_t i;
  for (i = 0;i<len;++i)
    direct_cons_putchar(s[i]);
  return 1;
}
