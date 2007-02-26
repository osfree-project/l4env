IMPLEMENTATION:

#include <cstdio>
#include <cctype>

#include "config.h"
#include "jdb.h"
#include "jdb_module.h"
#include "kmem.h"
#include "simpleio.h"
#include "space.h"
#include "static_init.h"

class Jdb_iomap : public Jdb_module
{
  static char     first_char;
  static Task_num taskno;
};

char     Jdb_iomap::first_char;
Task_num Jdb_iomap::taskno;


static void
Jdb_iomap::show()
{
  Space *s = Jdb::lookup_space(taskno);
  if (!s)
    {
      puts(" Invalid task");
      return;
    }

  // base addresses of the two IO bitmap pages
  Address bitmap_1, bitmap_2; 
  bitmap_1 = s->lookup(Kmem::io_bitmap, 0);
  bitmap_2 = s->lookup(Kmem::io_bitmap + Config::PAGE_SIZE, 0);

  Jdb::fancy_clear_screen();
  
  printf("\nIO bitmap for space %03x\n", (unsigned)s->space());
  if(bitmap_1 == 0xffffffff && bitmap_2 == 0xffffffff)
    { // no memory mapped for the IO bitmap
      putstr("No memory mapped");
      return;
    }
  else 
    {
      if(bitmap_1 != 0xffffffff)
	printf("First page mapped to  %08x; ", bitmap_1);
      else
	putstr("First page unmapped; ");

      if(bitmap_2 != 0xffffffff)
	printf("Second page mapped to %08x\n", bitmap_2);
      else
	puts("Second page unmapped");
    }

  puts("\nPorts assigned:");

  bool mapped= false;
  unsigned count=0;

  for(Mword i = 0; i < L4_fpage::IO_PORT_MAX; i++ )
    {
      if(s->io_lookup(i) != mapped)
	{
	  if(! mapped)
	    {
	      mapped = true;
	      printf("%04x-", i);
	    }
	  else
	    {
	      mapped = false;
	      printf("%04x, ", i-1);
	    }
	}
      if(mapped)
	count++;
    }
  if(mapped)
    printf("%04x, ", L4_fpage::IO_PORT_MAX -1);

  printf("\nPort counter: %d ", s->get_io_counter() );
  if(count == s->get_io_counter())
    puts("(correct)");
  else
    printf(" !! should be %d !!\n", count);

  return;	     
}

PUBLIC
Jdb_module::Action_code
Jdb_iomap::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      if (args == &first_char)
	{
	  taskno = Jdb::get_current_task();

	  if (isxdigit(first_char))
	    {
	      fmt       = "%3x";
	      args      = &taskno;
	      next_char = first_char;
	      return EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	}

      show();
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_iomap::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "r", "iomap", "%C",
	  "r[<taskno>]\tdisplay IO bitmap of current/given task",
	  &first_char),
    };
  return cs;
}

PUBLIC
int const
Jdb_iomap::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_iomap::Jdb_iomap()
  : Jdb_module("INFO")
{}

static Jdb_iomap jdb_iomap INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

