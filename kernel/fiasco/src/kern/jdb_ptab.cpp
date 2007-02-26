IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "jdb.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_table.h"
#include "kernel_console.h"
#include "kmem.h"
#include "keycodes.h"
#include "space.h"
#include "static_init.h"
#include "types.h"

class Jdb_ptab_m : public Jdb_module
{
  Task_num task;
  static char first_char;
};

class Jdb_ptab : public Jdb_table
{
  Address base;
  Address virt_base;
  int level;
  Task_num task;
  unsigned entries;
  unsigned char cur_pt_level;
  char dump_raw;
  
  static unsigned max_pt_level;

  static unsigned entry_valid(Mword entry, unsigned level);
  static unsigned entry_is_pt_ptr(Mword entry, unsigned level,
                                  unsigned *entries, unsigned *next_level);
  static Address entry_phys(Mword entry, unsigned level);
  static unsigned first_level_entries();
  
  void print_entry(Mword entry, unsigned level);

};

char Jdb_ptab_m::first_char;
//Task_num Jdb_ptab::task;

typedef Mword My_pte;			// shoud be replaced by
					// arch-dependent type
					
PUBLIC 
Jdb_ptab::Jdb_ptab(void *pt_base, Task_num task,
                   unsigned char pt_level = 0, 
                   unsigned entries = 0, Address virt_base = 0,
		   int level = 0)
: base((Address)pt_base), virt_base(virt_base), level(level), task(task),
  entries(entries), cur_pt_level(pt_level), dump_raw(0)
{
  if (!pt_level && entries == 0)
    this->entries = first_level_entries();
}

PUBLIC
unsigned 
Jdb_ptab::col_width(unsigned) const 
{ return 8; }

PUBLIC
unsigned
Jdb_ptab::cols() const
{ return 9; }


// available from the jdb_dump module
int jdb_dump_addr_task(Address addr, Task_num task, int level)
  __attribute__((weak));


PUBLIC
void 
Jdb_ptab::draw_entry(unsigned row, unsigned col)
{
  if (col==0)
    {
      printf(""L4_PTR_FMT"", virt(row, 1));
      return;
    }

  print_entry(*(My_pte*)(virt(row,col)), cur_pt_level);
}

PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_r(Address v)
{
  if (cur_pt_level == 0)
    v >>= Config::SUPERPAGE_SHIFT;
  else
    v >>= Config::PAGE_SHIFT;

  return v / (cols()-1);
}

PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_c(Address v)
{
  if (cur_pt_level == 0)
    v >>= Config::SUPERPAGE_SHIFT;
  else
    v >>= Config::PAGE_SHIFT;

  return (v % (cols()-1)) + 1;
}

PRIVATE
Address 
Jdb_ptab::disp_virt(unsigned row, unsigned col)
{
  unsigned e = (col-1) + (row * (cols()-1));
  if (cur_pt_level == 0)
    return e * Config::SUPERPAGE_SIZE;
  else
    return e * Config::PAGE_SIZE + virt_base;
}

PRIVATE
Address
Jdb_ptab::virt(unsigned row, unsigned col)
{
  unsigned e = (col-1) + (row * (cols()-1));
  return base + e * sizeof(Mword);
}

PUBLIC
void
Jdb_ptab::print_statline(unsigned row, unsigned col)
{
  if (cur_pt_level<max_pt_level)
    {
      Jdb::printf_statline("ptab", "<Space>=mode <CR>=goto ptab/superpage",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
  else // PT_MODE
    {
      Jdb::printf_statline("ptab", "<Space>=mode <CR>=goto page",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
}

PUBLIC
unsigned 
Jdb_ptab::key_pressed(int c, unsigned &row, unsigned &col)
{
  switch (c)
    {
    default:
      return Nothing;
      
    case KEY_CURSOR_HOME: // return to previous or go home
      if (level == 0)
	return Nothing;
      return Back;
    
    case ' ':
      dump_raw ^= 1;
      return Redraw;

    case KEY_RETURN:	// goto ptab/address under cursor
      if (level<=7)
	{
	  My_pte pt_entry = *(My_pte*)virt(row,col);
	  if (!entry_valid(pt_entry, cur_pt_level))
	    break;

	  Address pd_virt = (Address)
	    Mem_layout::phys_to_pmem(entry_phys(pt_entry, cur_pt_level));

	  unsigned next_level, entries;

	  if (cur_pt_level < max_pt_level
	      && entry_is_pt_ptr(pt_entry, cur_pt_level, &entries, &next_level))
	    {
	      Jdb_ptab pt_view((void *)pd_virt, task, next_level, entries,
		               disp_virt(row,col),level +1);
	      if (!pt_view.show(0,1))
		return Exit;
	      return Redraw;
	    }
	  else if (jdb_dump_addr_task != 0)
	    {
	      if (!jdb_dump_addr_task(disp_virt(row,col), task, level+1))
		return Exit;
	      return Redraw;
	    }
	}
      break;
    }
  
  return Handled;
}

PUBLIC
Jdb_module::Action_code
Jdb_ptab_m::action(int cmd, void *&args, char const *&fmt, int &next_char)
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
      
      Space *s = Jdb::lookup_space(task);
      if (!s)
	return Jdb_module::NOTHING;

      void *ptab_base;
      if (!(ptab_base = (void*)((Address)s->dir())))
	return Jdb_module::NOTHING;

      Jdb::clear_screen();
      Jdb_ptab pt_view(ptab_base, task);
      pt_view.show(0,1);
	
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_ptab_m::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "p", "ptab", "%C",
	  "p[<taskno>]\tshow pagetable of current/given task",
	  &first_char },
    };
  return cs;
}

PUBLIC
int const
Jdb_ptab_m::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_ptab_m::Jdb_ptab_m()
  : Jdb_module("INFO")
{}

static Jdb_ptab_m jdb_ptab_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

