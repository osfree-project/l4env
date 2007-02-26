IMPLEMENTATION:

#include <cstdio>

#include "jdb.h"
#include "jdb_input.h"
#include "jdb_screen.h"
#include "kernel_console.h"
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
  unsigned     i, j;
  int          c;
  
  if (page > Kip::k()->main_memory_high() / Config::PAGE_SIZE)
    {
      puts(" non extisting physical page");
      return;
    }

  Jdb::clear_screen();
  
  for (;;)
    {
      t = mapdb_instance()->physframe[page].tree.get();

      Jdb::cursor();
      printf("\033[K\n"
	     " mapping tree for page "L4_PTR_FMT" - header at " 
	     L4_PTR_FMT"\033[K\n\033[K\n",
	     page << 12,(Address)t);
#ifdef NDEBUG
      // If NDEBUG is active, t->_empty_count is undefined
      printf(" header info: "
	     "entries used: %u  free: --  total: %u  lock=%u\033[K\n"
	     "\033[K\n",
	     t->_count, t->number_of_entries(),
	     mapdb_instance()->physframe[page].lock.test());
  
      if (t->_count > t->number_of_entries())
	{
	  printf("\033[K\n"
		 "\033[K\n"
		 "  seems to be a wrong tree ! ...exiting");
	  // clear rest of page
	  for (i=6; i<Jdb_screen::height(); i++)
	    printf("\033[K\n");
	  return;
	}
#else
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
#endif

      m = t->mappings();

      for (i=0, j=1; i < t->_count; i++, j++, m++)
	{
	  unsigned indent = m->data()->depth;
	  if (indent > 10)
	    indent = 0;
      
	  Kconsole::console()->getchar_chance();

	  printf("%*u: %lx  va="L4_PTR_FMT"  task=%x  size=%u  depth=",
		 indent+3, i+1,
		 (Address)(m->data()), (Address)(m->data()->address << 12),
		 (Task_num)(m->data()->space), m->data()->size);
      
	  if (m->data()->depth == Depth_root)
	    printf("root");
	  else if (m->data()->depth == Depth_empty)
	    printf("empty");
	  else if (m->data()->depth == Depth_end)
	    printf("end");
	  else
	    printf("%u",m->data()->depth);
	  
	  puts("\033[K");

	  if (j >= Jdb_screen::height()-6)
	    {
	      printf(" any key for next side or <ESC>");
	      Jdb::cursor(Jdb_screen::height(), 33);
	      c = Jdb_core::getchar();
	      printf("\r\033[K");
	      if (c == KEY_ESC) 
		return;
	      j = 0;
	      Jdb::cursor(6, 1);
	    }
	}

      for (; j<Jdb_screen::height()-5; j++)
	puts("\033[K");

      Jdb::printf_statline("mapdb", "n=next p=revious", "_");

      for (bool redraw=false; !redraw; )
	{
	  Jdb::cursor(Jdb_screen::height(), 7);
	  switch (c = Jdb_core::getchar()) 
	    {
	    case 'n':
	    case KEY_CURSOR_DOWN:
	      if (++page > Kip::k()->main_memory_high() / Config::PAGE_SIZE)
      		page = 0;
	      redraw = true;
	      break;
	    case 'p':
	    case KEY_CURSOR_UP:
	      if (--page > Kip::k()->main_memory_high() / Config::PAGE_SIZE)
      		page =  Kip::k()->main_memory_high() / Config::PAGE_SIZE;
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
	{ 0, "m", "mapdb", " phys. frame number=%5x",
	  "m<frameno>\tshow mapping database starting at page",
	  &pagenum },
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

