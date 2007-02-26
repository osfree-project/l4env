INTERFACE:

#include "types.h"
#include "l4_types.h"


IMPLEMENTATION:

#include <cstdio>
#include <cctype>

#include "config.h"
#include "jdb.h"
#include "jdb_table.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"
#include "types.h"

class Jdb_dump : public Jdb_module, public Jdb_table
{
  enum
    {
      B_MODE = 'b',	// byte
      C_MODE = 'c',	// char
      D_MODE = 'd',	// word
    };

  static char show_adapter_memory;

public:
  unsigned cols() const { return 9; }
  unsigned rows() const { return ((unsigned)-1)/((cols()-1)*4) + 1; }
  void draw_entry(unsigned row, unsigned col);
  void print_statline();

private:
  int level;
  Task_num task;
  char dump_type;
};

char  Jdb_dump::show_adapter_memory;

// if we have the APIC module
extern int ignore_invalid_apic_reg_access
  __attribute__((weak));

// if we have the jdb_disasm module
extern int jdb_disasm_addr_task(Address addr, Task_num task, int level)
  __attribute__((weak));

PRIVATE inline 
Address 
Jdb_dump::virt(unsigned row, unsigned col)
{
  return (col-1) * 4 + row * (cols()-1) * 4;
}

PUBLIC
unsigned
Jdb_dump::col_width(unsigned col) const
{
  if (col && dump_type == C_MODE)
    return 4;
  else
    return 8; 
}

PUBLIC
void
Jdb_dump::print_statline(unsigned row, unsigned col)
{
  Jdb::printf_statline("dump", 
      (dump_type==D_MODE)
      ? "e=edit u=disasm D=dump <Space>=mode <CR>=goto addr"
      : "<Space>=mode",
      "%c<"L4_PTR_FMT"> task %-3x", dump_type, virt(row,col), task);
}
  
IMPLEMENT
void 
Jdb_dump::draw_entry(unsigned row, unsigned col)
{
  if (col == 0)
    {
      printf("%0*x", col_width(col), row * (cols()-1) * 4);
      return;
    }
	
  Address entry = (col-1) * 4 + row * (cols()-1) * 4;

  unsigned elb = 4;

  // prevent apic from getting confuse by invalid register accesses
  if (&ignore_invalid_apic_reg_access)
    ignore_invalid_apic_reg_access = 1;

  Mword mapped  = Jdb::peek_task(entry, task) != -1;
  Mword ram     = !Jdb::is_adapter_memory(entry, task);

  if (mapped)
    {
      if (ram || show_adapter_memory)
	{
	  Mword mword;
	  Jdb::peek_mword_task(entry, task, &mword);

	  if (dump_type==D_MODE)
	    {
	      if (mword == 0)
		printf("%8d", 0);
	      else if (mword == (Mword)-1)
		printf("%8d", -1);
	      else
		printf(""L4_PTR_FMT"", mword);
	    } 
	  else if (dump_type==B_MODE)
	    {
	      for (Mword u=0; u<elb; u++)
		{
		  Unsigned8 b = (mword >> (8*u)) & 0xff;
		  printf("%02x", b);
		}
	    }
	  else if (dump_type==C_MODE)
	    {
	      for (Mword u=0; u<elb; u++)
		{
		  Unsigned8 b = (mword >> (8*u)) & 0xff;
		  putchar(b>=32 && b<=126 ? b : '.');
		}
	    }
	}
      else // is_adapter_memory
	{
	  if (dump_type == C_MODE)
	    putstr("----");
	  else
	    putstr("--------");
	}
    }
  else // !mapped
    {
      if (dump_type == C_MODE)
	putstr("~~~~");
      else
        putstr("........");
    }
  
  if (&ignore_invalid_apic_reg_access)
    ignore_invalid_apic_reg_access = 0;
}

PUBLIC
Jdb_module::Action_code
Jdb_dump::dump(Address virt, Task_num task, int level)
{
  int old_l = this->level;
  this->level = level;
  this->task = task;
  dump_type = D_MODE;
  if (!level)
    Jdb::clear_screen();
  
  unsigned row = virt / ((cols()-1) * 4);
  unsigned col = (virt % ((cols()-1) * 4)) / 4 + 1;
  bool r = show(row, col);
  this->level = old_l;
  return r ? EXTRA_INPUT : NOTHING;
}

PUBLIC
bool 
Jdb_dump::edit_entry(unsigned row, unsigned col, unsigned cx, unsigned cy)
{
  Address entry = virt(row,col);

  Mword mword;
  if (!Jdb::peek_mword_task(entry, task, &mword))
    return false;

  putstr("        ");
  Jdb::printf_statline("dump", "<CR>=commit change",
		       "edit <"L4_PTR_FMT"> = "L4_PTR_FMT"", entry, mword);

  Jdb::cursor(cy, cx);
  Mword new_mword;
  if (Jdb_input::get_mword(&new_mword, 8, 16))
    Jdb::poke_mword_task(entry, task, new_mword);

  return true; // redraw
}

PUBLIC
unsigned 
Jdb_dump::key_pressed(int c, unsigned &row, unsigned &col)
{
  switch (c)
    {
    default:
      return Nothing;
      
    case KEY_CURSOR_HOME: // return to previous or go home
      if (level == 0)
	{
	  Address v = virt(row, col);
	  if (v == 0)
	    return Handled;

	  if ((v & ~Config::PAGE_MASK) == 0)
	    row -= Config::PAGE_SIZE / 32;
	  else
	    {
	      col = 1;
	      row = (v & Config::PAGE_MASK) / 32;
	    }
	  return Redraw;
	}
      return Back;

    case KEY_CURSOR_END:
	{
	  Address v = virt(row, col);
	  if ((v & ~Config::PAGE_MASK) >> 2 == 0x3ff)
	    row += Config::PAGE_SIZE / 32;
	  else
	    {
	      col = 8;
	      row = ((v & Config::PAGE_MASK) + Config::PAGE_SIZE - 4) / 32;
	    }
	}
      return Redraw;
    
    case 'D':
      if (Kconsole::console()->find_console(Console::GZIP))
	{
	  Address low_addr, high_addr;

	  Jdb::cursor(Jdb_screen::height(), 27);
	  putchar('[');
	  Jdb::clear_to_eol();

	  if (Jdb_input::get_mword(&low_addr, 8, 16))
	    {
	      putchar('-');
	      if (Jdb_input::get_mword(&high_addr, 8, 16))
		{
		  unsigned l_row = low_addr  / ((cols()-1) * 4);
		  unsigned l_col = (low_addr % ((cols()-1) * 4)) / 4;
		  unsigned h_row = high_addr / ((cols()-1) * 4);

		  if (low_addr <= high_addr)
		    {
		      Mword lines = h_row - l_row;
		      if (lines < 1)
			lines = 1;
		      // enable gzip console
		      Kconsole::console()->
			start_exclusive(Console::GZIP);
		      char old_mode = dump_type;
		      dump_type = D_MODE;
		      draw_table(l_row, l_col, lines, cols());
		      dump_type = old_mode;
		      Kconsole::console()->
			end_exclusive(Console::GZIP);
		    }
		}
	    }
	  return Redraw;
	}
      return Handled;
    
    case ' ': // change viewing mode
      switch (dump_type)
	{
	case D_MODE: dump_type=B_MODE; return Redraw;
	case B_MODE: dump_type=C_MODE; return Redraw;
	case C_MODE: dump_type=D_MODE; return Redraw;
	}
      break;

    case KEY_TAB:
      show_adapter_memory = !show_adapter_memory;
      return Redraw;

    case KEY_RETURN: // goto address under cursor
      if (level<=7 && dump_type==D_MODE)
	{
	  Address virt1;
	  if (Jdb::peek_addr_task(virt(row,col), task, &virt1) != -1)
	    {
	      if (!dump(virt1, task, level +1))
		  return Exit;
	      return Redraw;
	    }
	}
      break;

    case 'u': // disassemble using address the cursor points to
      if (jdb_disasm_addr_task && level<=7 && dump_type == D_MODE)
	{
	  Address virt1;
	  if (Jdb::peek_addr_task(virt(row,col), task, &virt1) != -1)
	    {
	      Jdb::printf_statline("dump", "<CR>=disassemble here", 
				   "u[address="L4_PTR_FMT" task=%x] ", 
				   virt1, task);
	      int c1 = Jdb_core::getchar();
	      if (c1 != KEY_RETURN && c1 != ' ')
		{
		  Jdb::printf_statline("dump", 0, "u");
		  Jdb::execute_command("u", c1);
		  return Exit;
		}

	      return jdb_disasm_addr_task(virt1, task, level+1) 
		? Redraw
		: Exit;
	    }
	}
      return Handled;

    case 'e': // poke memory
      if (dump_type == D_MODE)
	return Edit;
      break;
    }

  return Handled;

}

PUBLIC
Jdb_module::Action_code
Jdb_dump::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      Jdb_module::Action_code code;
      
      code = Jdb_input_task_addr::action(args, fmt, next_char);
      if (code == Jdb_module::NOTHING 
	  && Jdb_input_task_addr::task != (Task_num)-1)
	{
	  Address addr  = Jdb_input_task_addr::addr;
	  Task_num task = Jdb_input_task_addr::task;
	  if (addr == (Address)-1)
	    addr = 0;

	  return dump(addr,task,0);
	}

      if (code != ERROR)
	return code;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_dump::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "d", "dump", "%C",
	  "d[t<taskno>]<addr>\tdump memory of given/current task at <addr>",
	  &Jdb_input_task_addr::first_char },
    };
  return cs;
}

PUBLIC
int const
Jdb_dump::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_dump::Jdb_dump()
  : Jdb_module("INFO"), dump_type(D_MODE)
{}

static Jdb_dump jdb_dump INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

int
jdb_dump_addr_task(Address addr, Task_num task, int level)
{ return jdb_dump.dump(addr, task, level); }

