#include <libc_backend.h>

#include "console.h"

int __libc_backend_outs( const char *s, size_t len )
{
  for(size_t i = 0;i<len;++i) {
    leg_putchar(s[i]);
  }
  return 1;
}


int __libc_backend_ins( char *s, size_t len )
{
  size_t act = 0;
  for(; act < len; act++) {
    s[act]=leg_getchar();
    if(s[act]=='\n') break;
  }
  return act;
}

