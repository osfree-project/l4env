IMPLEMENTATION:

#include <cstdio>

#include "jdb.h"
#include "jdb_input.h"
#include "jdb_screen.h"
#include "kmem.h"
#include "keycodes.h"
#include "mapdb.h"
#include "mapdb_i.h"
#include "map_util.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_mapdb : public Jdb_module
{
  static Mword pagenum;
};


Mword Jdb_mapdb::pagenum;

static
void
Jdb_mapdb::show(Mword page)
{
  Mapping_tree *t;
  Mapping      *m;
  Mword        i;
  int          c;
  
  if (page > Kmem::info()->main_memory_high() / Config::PAGE_SIZE)
    {
      puts(" non extisting physical page");
      return;
    }

  Jdb::fancy_clear_screen();
  
  for (;;)
    {
      t = mapdb_instance()->physframe[page].tree.get();

      Jdb::cursor();
      printf("\033[K\n"
	     " mapping tree for page %08x - header at %08x\033[K\n"
	     "\033[K\n",
	     page << 12,(unsigned)t);
      printf(" header info: "
	     "entries used: %u  free: %u  total: %u  lock=%u\033[K\n"
	     "\033[K\n",
	     t->_count,t->_empty_count, t->number_of_entries(),
	     mapdb_instance()->physframe[page].lock.test());
  
      if (t->_count + t->_empty_count > t->number_of_entries())
	{
	  printf("\033[K\n"
		 "\033[K\n"
		 "  seems to be a wrong tree ! ...exiting");
	  // clear rest of page
	  for (i=6; i<Jdb_screen::height(); i++)
	    printf("\033[K\n");
	  return;
	}

      m = t->mappings();

      for (i=0; i < t->number_of_entries(); i++, m++)
	{
	  unsigned indent = m->data()->depth;
	  if (indent > 10)
	    indent = 0;
      
	  printf("%*u: %x  va=%08x  task=%x  size=%u  depth=",
		 indent+3, i+1,
		 (Address)(m->data()), (Task_num)(m->data()->address << 12),
		 (Mword)(m->data()->space()), m->data()->size);
      
	  if (m->data()->depth == Depth_root)
	    printf("root");
	  else if (m->data()->depth == Depth_empty)
	    printf("empty");
	  else if (m->data()->depth == Depth_end)
	    printf("end");
	  else
	    printf("%u",m->data()->depth);
	  
	  puts("\033[K");

	  if (i % 18 == 17)
	    {
	      printf(" any key for next side or <ESC>");
	      Jdb::cursor(Jdb_screen::height(), 33);
	      c = getchar();
	      printf("\r\033[K");
	      if (c == KEY_ESC) 
		return;
	      Jdb::cursor(6, 1);
	    }
	}

      for (; i<18; i++)
	puts("\033[K");

      Jdb::printf_statline("m%73s", "n=next p=revious");

      for (bool redraw=false; !redraw; )
	{
	  Jdb::cursor(Jdb_screen::height(), Jdb::LOGO+1);
	  switch (c = getchar()) 
	    {
	    case 'n':
	    case KEY_CURSOR_DOWN:
	      if (++page > Kmem::info()->main_memory_high() / Config::PAGE_SIZE)
      		page = 0;
	      redraw = true;
	      break;
	    case 'p':
	    case KEY_CURSOR_UP:
	      if (--page > Kmem::info()->main_memory_high() / Config::PAGE_SIZE)
      		page =  Kmem::info()->main_memory_high() / Config::PAGE_SIZE;
	      redraw = true;
	      break;
	    case KEY_ESC:
	      Jdb::abort_command();
	      return;
	    default:
	      if (Jdb::is_toplevel_cmd(c)) 
		return;
	    }
	}
    }
}

PUBLIC
Jdb_module::Action_code
Jdb_mapdb::action(int cmd, void *&, char const *&, int &)
{
  if (cmd == 0)
    show(pagenum);

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_mapdb::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "m", "mapdb", " phys. frame number=%5x",
	  "m<frameno>\tshow mapping database starting at page",
	  &pagenum),
    };
  return cs;
}

PUBLIC
int const
Jdb_mapdb::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_mapdb::Jdb_mapdb() : Jdb_module("INFO")
{}

static Jdb_mapdb jdb_mapdb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

