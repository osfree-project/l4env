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
public:
  Jdb_mapdb() FIASCO_INIT;
private:
  static Mword pagenum;
  static char  subcmd;
};

char  Jdb_mapdb::subcmd;
Mword Jdb_mapdb::pagenum;

static 
const char*
size_str (Mword size)
{
  static char scratchbuf[6];
  unsigned mult = 0;
  while (size >= 1024)
    {
      size >>= 10;
      mult++;
    }
  snprintf (scratchbuf, 6, "%u%c", unsigned(size), "BKMGTPX"[mult]);
  return scratchbuf;
}

static
bool
Jdb_mapdb::show_tree(Treemap* pages, Mword address, 
		     unsigned &screenline, unsigned indent = 1)
{
  Mword         page = address / pages->_page_size;
  Physframe*    f = pages->frame(page);
  Mapping_tree* t = f->tree.get();
  unsigned      i;
  int           c;

  if (! t)
    {
      printf(" no mapping tree registered for frame number 0x%x\033[K\n",
	     (unsigned) page);
      screenline++;
      return true;
    }

  printf(" mapping tree for %s-page "L4_PTR_FMT" of task %x - header at " 
	 L4_PTR_FMT"\033[K\n",
	 size_str (pages->_page_size),
	 pages->vaddr(t->mappings()), t->mappings()[0].space(), (Address)t);
#ifdef NDEBUG
  // If NDEBUG is active, t->_empty_count is undefined
  printf(" header info: "
	 "entries used: %u  free: --  total: %u  lock=%u\033[K\n",
	 t->_count, t->number_of_entries(),
	 f->lock.test());
  
  if (t->_count > t->number_of_entries())
    {
      printf("\033[K\n"
	     "\033[K\n"
	     "  seems to be a wrong tree ! ...exiting");
      // clear rest of page
      for (i=6; i<Jdb_screen::height(); i++)
	printf("\033[K\n");
      return false;
    }
#else
  printf(" header info: "
	 "entries used: %u  free: %u  total: %u  lock=%u\033[K\n",
	 t->_count, t->_empty_count, t->number_of_entries(),
	 f->lock.test());
  
  if (unsigned (t->_count) + t->_empty_count > t->number_of_entries())
    {
      printf("\033[K\n"
	     "\033[K\n"
	     "  seems to be a wrong tree ! ...exiting");
      // clear rest of page
      for (i=6; i<Jdb_screen::height(); i++)
	printf("\033[K\n");
      return false;
    }
#endif

  Mapping* m = t->mappings();
  
  screenline += 2;

  for (i=0; i < t->_count; i++, m++)
    {
      Kconsole::console()->getchar_chance();
      
      if (m->depth() == Mapping::Depth_submap)
	printf("%*u: %lx  subtree@"L4_PTR_FMT,
	       indent + m->parent()->depth() > 10 
	         ? 0 : indent + m->parent()->depth(), 
	       i+1, (Address) m->data(), (Mword) m->submap());
      else
	{
	  printf("%*u: %lx  va="L4_PTR_FMT"  task=%x  depth=",
		 indent + m->depth() > 10 ? 0 : indent + m->depth(), 
		 i+1, (Address) m->data(),
		 pages->vaddr(m),
		 (Task_num)(m->space()));
	  
	  if (m->depth() == Mapping::Depth_root)
	    printf("root");
	  else if (m->depth() == Mapping::Depth_empty)
	    printf("empty");
	  else if (m->depth() == Mapping::Depth_end)
	    printf("end");
	  else
	    printf("%lu", static_cast<unsigned long>(m->depth()));
	}

      puts("\033[K");
      screenline++;
      
      if (screenline >= (m->depth() == Mapping::Depth_submap 
			 ? Jdb_screen::height() - 3
			 : Jdb_screen::height()))
	{
	  printf(" any key for next page or <ESC>");
	  Jdb::cursor(screenline, 33);
	  c = Jdb_core::getchar();
	  printf("\r\033[K");
	  if (c == KEY_ESC) 
	    return false;
	  screenline = 3;
	  Jdb::cursor(3, 1);
	}

      if (m->depth() == Mapping::Depth_submap)
	{
	  if (! Jdb_mapdb::show_tree(m->submap(), 
				     address & (pages->_page_size - 1),
				     screenline, indent + m->parent()->depth()))
	    return false;	     
	}
    }

  return true;
}

static
Unsigned64
Jdb_mapdb::end_address (Mapdb* mapdb)
{
  return mapdb->_treemap->end_addr();
}

static
void
Jdb_mapdb::show (Mword page, char which_mapdb)
{
  unsigned     j;
  int          c;
  
  Jdb::clear_screen();

  for (;;)
    {
      Mapdb* mapdb;
      const char* type;
      Mword page_shift, super_inc;

      switch (which_mapdb)
	{
	case 'm':
	  type = "Phys frame";
	  mapdb = mapdb_instance();
	  page_shift = Config::PAGE_SHIFT;
	  super_inc = Config::SUPERPAGE_SIZE / Config::PAGE_SIZE;
	  break;
#ifdef CONFIG_IO_PROT
	case 'i':
	  type = "I/O port";
	  mapdb = io_mapdb_instance();
	  page_shift = 0;
	  super_inc = 0x100;
	  break;
#endif
#ifdef CONFIG_TASK_CAPS
	case 't':
	  type = "Task";
	  mapdb = cap_mapdb_instance();
	  page_shift = 0;
	  super_inc = 1;
	  break;
#endif
	default:
	  return;
	}

      if (! mapdb->valid_address (page << page_shift))
	page = 0;

      Jdb::cursor();
      printf ("%s "L4_PTR_FMT"\033[K\n\033[K\n", 
	      type, page << page_shift);
	 
      j = 3;
      
      if (! Jdb_mapdb::show_tree (mapdb->_treemap,
				  (page << page_shift) 
				    - mapdb->_treemap->_page_offset,
				  j))
	return;

      for (; j<Jdb_screen::height(); j++)
	puts("\033[K");

      static char prompt[] = "mapdb[m]";
      prompt[6] = which_mapdb;

      Jdb::printf_statline(prompt, 
			   "n=next p=previous N=nextsuper P=prevsuper", "_");

      for (bool redraw=false; !redraw; )
	{
	  Jdb::cursor(Jdb_screen::height(), 10);
	  switch (c = Jdb_core::getchar()) 
	    {
	    case 'n':
	    case KEY_CURSOR_DOWN:
	      if (! mapdb->valid_address(++page << page_shift))
      		page = 0;
	      redraw = true;
	      break;
	    case 'p':
	    case KEY_CURSOR_UP:
	      if (! mapdb->valid_address(--page << page_shift))
      		page = (end_address (mapdb) - 1) >> page_shift;
	      redraw = true;
	      break;
	    case 'N':
	    case KEY_PAGE_DOWN:
	      page = (page + super_inc) & ~(super_inc - 1);
	      if (! mapdb->valid_address(page << page_shift))
      		page = 0;
	      redraw = true;
	      break;
	    case 'P':
	    case KEY_PAGE_UP:
	      page = (page - super_inc) & ~(super_inc - 1);
	      if (! mapdb->valid_address(page << page_shift))
      		page = (end_address (mapdb) - 1) >> page_shift;
	      redraw = true;
	      break;
	    case ' ':
	      if (which_mapdb == 'm')
#ifdef CONFIG_IO_PROT
		which_mapdb = 'i';
	      else if (which_mapdb == 'i')
#endif
#ifdef CONFIG_TASK_CAPS
		which_mapdb = 't';
	      else if (which_mapdb == 't')
#endif
		which_mapdb = 'm';
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
Jdb_mapdb::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  static char which_mapdb = 'm';

  if (cmd != 0)
    return NOTHING;

  if (args == (void*) &subcmd)
    {
      switch (subcmd)
	{
	default:
	  return NOTHING;

	case '\r':
	case ' ':
	  goto doit;

	case '0' ... '9':
	case 'a' ... 'f':
	case 'A' ... 'F':
	  which_mapdb = 'm';
	  fmt = " frame: "L4_FRAME_INPUT_FMT;
	  args = &pagenum;
	  next_char = subcmd;
	  return EXTRA_INPUT_WITH_NEXTCHAR;
	  
	case 'm':
	  fmt = " frame: "L4_FRAME_INPUT_FMT;
	  break;

#ifdef CONFIG_IO_PROT
	case 'i':
	  fmt = " port: "L4_FRAME_INPUT_FMT;
	  break;
#endif

#ifdef CONFIG_TASK_CAPS
	case 't':
	  fmt = " task: "L4_FRAME_INPUT_FMT;
	  break;
#endif
	}

      which_mapdb = subcmd;
      args = &pagenum;
      return EXTRA_INPUT;
    }

  else if (args != (void*) &pagenum)
    return NOTHING;

 doit:
  show(pagenum, which_mapdb);
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_mapdb::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "m", "mapdb", "%c",
	  "m[it]<addr>\tshow [I/O,task] mapping database starting at address",
	  &subcmd },
    };
  return cs;
}

PUBLIC
int
Jdb_mapdb::num_cmds() const
{
  return 1;
}

IMPLEMENT
Jdb_mapdb::Jdb_mapdb()
  : Jdb_module("INFO")
{}

static Jdb_mapdb jdb_mapdb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

