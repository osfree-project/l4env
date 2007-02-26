IMPLEMENTATION:

#include <cstring>
#include <cstdio>

#include "config.h"
#include "console.h"
#include "gzip.h"
#include "kernel_console.h"
#include "kmem_alloc.h"
#include "panic.h"
#include "static_init.h"

class Jdb_gzip : public Console
{
  static const unsigned heap_pages = 34;
  char active;
  char init_done;
};

static void
Jdb_gzip::raw_write(const char *s, size_t len)
{
  Kconsole::console()->gzip_write(s, len);
}

PUBLIC inline NOEXPORT
void
Jdb_gzip::enable()
{
  if (!init_done)
    {
      char *heap = (char*)Kmem_alloc::allocator()->unaligned_alloc(heap_pages);
      if (!heap)
	panic("No memory for gzip heap");
      gz_init(heap, heap_pages * Config::PAGE_SIZE, raw_write);
    }

  gz_open("jdb.gz");
  active = 1;
}

PUBLIC inline NOEXPORT
void
Jdb_gzip::disable()
{
  if (active)
    {
      gz_close();
      active = 0;
    }
}

PUBLIC
int
Jdb_gzip::write(char const *str, size_t len)
{
  if (len == 10 && !memcmp(str, "START_GZIP", 10))
    enable();
  else if (len == 9 && !memcmp(str, "STOP_GZIP", 10))
    disable();
  else if (active)
    gz_write(str, len);

  return len;
}

PUBLIC static
Console* const
Jdb_gzip::console()
{
  static Jdb_gzip c;
  return &c;
}


PUBLIC
char const *Jdb_gzip::next_attribute( bool restart = false ) const
{
  static char const *attribs[] = { "gzip", "out", 0 };
  static unsigned pos = 0;
  if(restart)
    pos = 0;
  
  if(pos < 2)
    return attribs[pos++];
  else
    return 0;
}


static void
jdb_gzip_init()
{
  Kconsole::console()->register_gzip_console(Jdb_gzip::console());
}

STATIC_INITIALIZER_P(jdb_gzip_init, UART_INIT_PRIO);

