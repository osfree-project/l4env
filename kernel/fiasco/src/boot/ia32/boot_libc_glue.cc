#include <libc_backend.h>

#include "boot_direct_cons.h"

int __libc_backend_outs( const char *s, size_t len )
{
  size_t i;
  for (i = 0;i<len;++i)
    direct_cons_putchar(s[i]);
  return 1;
}


int __libc_backend_ins( char *s, size_t len )
{
  size_t act = 0;
  for(; act < len; act++)
    {
      s[act]=direct_cons_getchar();
      if(s[act]=='\n') break;
    }
  return act;
}
