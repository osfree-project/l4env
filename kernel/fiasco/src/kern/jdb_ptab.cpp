IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "jdb.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "kmem.h"
#include "keycodes.h"
#include "space.h"
#include "static_init.h"
#include "types.h"

class Jdb_ptab : public Jdb_module
{
  enum
    {
      PD_MODE      = 'e',		// page directory
      PT_MODE      = 'f'		// page table
    };

  static char     first_char;
  static char     dump_raw;
  static Task_num task;
};

char     Jdb_ptab::first_char;
char     Jdb_ptab::dump_raw;
Task_num Jdb_ptab::task;

typedef Mword Pt_entry;			// shoud be replaced by
					// arch-dependent type

// available from the jdb_dump module
int jdb_dump_addr_task(Address addr, Task_num task, int level)
  __attribute__((weak));

// We assume that we can touch into the virtual memory to read
// a page table entry. This is at least true for IA32 and UX where
// the current page table is synchronized with the kernel page table
// on kernel debugger entry.
static
Jdb_module::Action_code
Jdb_ptab::dump(char dump_type, Address ptab_base, Address disp_virt,
	       Task_num task, int level)
{
  Address addx = 0;			// current cursor x
  Address addy = 0;			// current cursor y
  Address absy = 0;			// absolute y relative to entry_base
  Address lines = Jdb_screen::height()-1;// # lines to display
  Address max_absy;			// number of lines
  Address ptab_entry, old_entry = 0;
  bool old_entry_set = false;
  Mword line_offs = 32;
  Mword line_page = Config::PAGE_SIZE / line_offs;

  if (level==0)
    Jdb::fancy_clear_screen();

  Space *s = Jdb::lookup_space(task);
  if (!s)
    return NOTHING;

  max_absy = line_page - lines;

  if (dump_type==PD_MODE)
    {
      if (!(ptab_base = (Address)s->dir()))
	return NOTHING;
      disp_virt = 0;
    }
  else // PT_MODE
    {
      ptab_base &= Config::PAGE_MASK;
      disp_virt &= Config::PAGE_MASK;
    }


 screen:

  Jdb::cursor();
  dump_ptab(lines, ptab_base + absy*line_offs);

  for (bool redraw=false; ; )
    {
      ptab_entry = ptab_base + (absy+addy)*line_offs + addx*sizeof(Pt_entry);

      if (old_entry_set && (ptab_entry != old_entry))
	{
	  switch (dump_type)
	    {
	    case PD_MODE:
	      disp_virt += (ptab_entry-old_entry)
			    *(Config::SUPERPAGE_SIZE/sizeof(Pt_entry));
	      break;
	    case PT_MODE:
	      disp_virt += (ptab_entry-old_entry)
			    *(Config::PAGE_SIZE/sizeof(Pt_entry));
	      break;
	    }
	}
      old_entry     = ptab_entry;
      old_entry_set = true;

      if (dump_type==PD_MODE)
	{
	  Jdb::printf_statline("p<%08x> task %-3x %53s",
			       disp_virt, task,
			       "<Space>=mode <CR>=goto ptab/superpage");
	}
      else // PT_MODE
	{
	  Jdb::printf_statline("p<%08x> task %-3x %53s",
	 		       disp_virt, task, 
			       "<Space>=mode <CR>=goto page");
	}

      // mark next value
      Jdb::cursor(addy+1, (2*sizeof(Pt_entry)+1)*addx + 2*sizeof(Address) + 2);
      int c = Kconsole::console()->getchar();

      switch (c)
	{
	case KEY_CURSOR_HOME: // return to previous or go home
	  if (level == 0)
	    {
	      Mword old_absy = absy;
	      absy = addx = addy = 0;
	      if (absy != old_absy)
		goto screen;
	      continue;
	    }
	  return GO_BACK;

	case KEY_CURSOR_END:
	    {
	      Mword old_absy = absy;
	      absy = max_absy;
	      addx = 7;
	      addy = lines-1;
	      if (absy != old_absy)
		goto screen;
	      continue;
	    }
	}

      if (!Jdb::std_cursor_key(c, lines, max_absy,
			       &absy, &addy, &addx, &redraw))
	{
	  switch(c)
	    {
	    case ' ':
	      dump_raw ^= 1;
	      redraw = true;
	      break;

	    case KEY_RETURN:	// goto ptab/address under cursor
	      if (level<=7)
		{
		  Pt_entry pt_entry = *(Pt_entry*)ptab_entry;

		  if (!Jdb::pt_entry_valid(pt_entry))
		    break;

		  Address pd_virt = (Address)
		    Kmem::phys_to_virt(Jdb::pt_entry_phys(pt_entry));

		  if (dump_type==PD_MODE
		      && Jdb::pd_entry_is_ptr_to_pt(pt_entry))
		    {
		      if (!dump(PT_MODE, pd_virt, disp_virt, task, level+1))
			return NOTHING;
		      redraw = true;
		    }
		  else if (jdb_dump_addr_task != 0)
		    {
		      if (!jdb_dump_addr_task(disp_virt, task, level+1))
			return NOTHING;
		      redraw = true;
		    }
		}
	      break;

	    case KEY_ESC:
	      Jdb::abort_command();
	      return NOTHING;

	    default:
	      if (Jdb::is_toplevel_cmd(c)) 
		return NOTHING;
	    }
	}
      if (redraw)
	goto screen;
    }
}

static void
Jdb_ptab::dump_ptab(Mword lines, Address virt)
{
  for (Mword i=0; i<lines; i++, virt += 32)
    {
      Kconsole::console()->getchar_chance();
      printf("%08x", virt);
      for (Mword x=0; x<32/sizeof(Pt_entry); x++)
	{
	  putchar(x == 0 ? ':' : ' ');
	  if (dump_raw)
	    printf("%08x", *(Pt_entry*)(virt + x*sizeof(Pt_entry)));
	  else
	    Jdb::dump_pt_entry(*(Pt_entry*)(virt + x*sizeof(Pt_entry)));
	}
      putchar('\n');
    }
}

PUBLIC
Jdb_module::Action_code
Jdb_ptab::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      if (args == &first_char)
	{
	  if (first_char != KEY_RETURN && first_char != ' ')
	    {
	      fmt       = "%3x";
	      args      = &task;
	      next_char = first_char;
	      return EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	  else
	    {
	      task = Jdb::get_current_task();
	    }
	}
      else if (args == &task)
	{
	  if (!Jdb::is_valid_task(task))
	    {
	      puts(" invalid task");
	      return NOTHING;
	    }
	}
      else
	return NOTHING;

      dump(PD_MODE, 0, 0, task, 0);
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_ptab::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "p", "ptab", "%C",
	  "p[<taskno>]\tshow pagetable of current/given task",
	  &first_char),
    };
  return cs;
}

PUBLIC
int const
Jdb_ptab::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_ptab::Jdb_ptab()
  : Jdb_module("INFO")
{}

static Jdb_ptab jdb_ptab INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

