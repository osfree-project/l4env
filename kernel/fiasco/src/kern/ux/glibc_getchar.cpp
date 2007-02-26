IMPLEMENTATION:

#include "console.h"
#include "filter_console.h"
#include "glibc_getchar.h"
#include "kernel_console.h"
#include "static_init.h"
#include "stdio.h"

class Glibc_getchar : public Console
{};

PUBLIC
int
Glibc_getchar::getchar( bool /*blocking*/ )
{
  return ::getchar();
}

PUBLIC
int
Glibc_getchar::write( char const * /*str*/, size_t /*len*/ )
{
  return 1;
}

PUBLIC
char const *
Glibc_getchar::next_attribute (bool restart = false) const
{
  static char const *attribs[] = { "ux", "in" };
  static unsigned int pos = 0;
  
  if (restart)
    pos = 0;
  
  if (pos < sizeof (attribs) / sizeof (*attribs))
    return attribs[pos++];
    
  return 0;
}

PUBLIC static
Console* const
Glibc_getchar::instance()
{
  static Glibc_getchar c;
  return &c;
}

static void
glibc_flt_getchar_init()
{
  static Filter_console fcon(Glibc_getchar::instance(), 0);

  Kconsole::console()->register_console(&fcon);
}

STATIC_INITIALIZER_P(glibc_flt_getchar_init, UART_INIT_PRIO);
