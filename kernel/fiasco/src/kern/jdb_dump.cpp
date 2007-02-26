INTERFACE:

#include "types.h"
#include "l4_types.h"


IMPLEMENTATION:

#include <cstdio>
#include <cctype>

#include "config.h"
#include "jdb.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"
#include "types.h"

class Jdb_dump : public Jdb_module
{
  enum
    {
      B_MODE = 'b',	// byte
      C_MODE = 'c',	// char
      D_MODE = 'd',	// word
    };

  static char show_adapter_memory;
};

char  Jdb_dump::show_adapter_memory;

// if we have the APIC module
extern int ignore_invalid_apic_reg_access
  __attribute__((weak));

// if we have the jdb_disasm module
extern int jdb_disasm_addr_task(Address addr, Task_num task, int level)
  __attribute__((weak));

PUBLIC static
Jdb_module::Action_code
Jdb_dump::dump(Address virt, Task_num task, int level)
{
  Address addx;				// current cursor x
  Address addy;				// current cursor y
  Address absy;				// absolute y relative to entry_base
  Address max_absy;			// number of lines
  Address entry, old_entry = 0;
  Address entry_base;
  bool old_entry_set = false;
  Mword elc;				// entry_lenght in characters
  Mword elb = sizeof(Mword);		// entry_lenght in bytes
  Mword epl = 32 / elb;			// entries per line
  Mword bol = 2*sizeof(Address)+1;	// begin of line
  Mword lines = Jdb_screen::height()-1;	// # lines to display
  Mword line_offs = epl*elb;
  Mword line_mask = line_offs-1;
  Mword line_page = Config::PAGE_SIZE / line_offs;
  int dump_type = D_MODE;

  if (level==0)
    Jdb::fancy_clear_screen();

  addx       = (virt & line_mask) / elb;
  addy       = 0;
  absy       = virt / line_offs;
  max_absy   = (0 - lines*line_offs) / line_offs;
  entry_base = 0;
  virt      &= ~(elb-1);

  if (absy > max_absy)
    {
      Mword diff = absy-max_absy;
      absy      -= diff;
      addy      += diff;
    }

 dump_mode:
 
  elc = (dump_type==C_MODE) ? 4 : 8;

 screen:

  Jdb::cursor();
  dump_memory(lines, dump_type, entry_base + absy*line_offs, task, elb, epl);

  for (bool redraw=false; ; )
    {
      entry = entry_base + (absy+addy)*line_offs + addx*elb;

      if (old_entry_set && (entry != old_entry))
	virt += entry-old_entry;

      old_entry     = entry;
      old_entry_set = true;

      Jdb::printf_statline("%c<%08x> task %-3x %53s", dump_type, virt, 
	  task, (dump_type==D_MODE)
			? "e=edit u=disasm D=dump <Space>=mode <CR>=goto addr"
			: "<Space>=mode");
      
      // mark next value
      Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
      int c = getchar();

      switch (c)
	{
	case KEY_CURSOR_HOME: // return to previous or go home
	  if (level == 0)
	    {
	      Mword old_absy = absy;
	      if (absy > 0)
		absy = (absy + addy - 1 + ((addx+6)/7)) & ~(line_page-1);
	      addx = 0;
	      addy = 0;
	      if (absy != old_absy)
		goto screen;
	      continue;
	    }
	  return GO_BACK;

	case KEY_CURSOR_END:
	    {
	      Mword old_absy = absy;
	      if (absy < max_absy)
		absy = ((absy + addy + addx/7) & ~(line_page-1)) 
		     + line_page-lines;
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
	    case 'D':
	      if (Kconsole::console()->gzip_available())
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
			  low_addr  &= ~line_mask;
			  high_addr += 2*line_offs-1;
			  high_addr &= ~line_mask;

			  if (low_addr <= high_addr)
			    {
			      Mword lines = (high_addr-low_addr-1) / line_offs;
			      if (lines < 1)
				lines = 1;
			      Kconsole::console()->gzip_enable();
			      dump_memory(lines, D_MODE, low_addr, task, 
					  elb, epl);
			      Kconsole::console()->gzip_disable();
			    }
			}
		    }
		}
	      redraw = true;
	      break;

	    case ' ': // change viewing mode
	      switch (dump_type)
		{
		case D_MODE: dump_type=B_MODE; goto dump_mode;
		case B_MODE: dump_type=C_MODE; goto dump_mode;
		case C_MODE: dump_type=D_MODE; goto dump_mode;
		}
	      break;

	    case KEY_TAB:
	      show_adapter_memory = !show_adapter_memory;
	      redraw = true;
	      break;

	    case KEY_RETURN: // goto address under cursor
	      if (level<=7 && dump_type==D_MODE)
		{
		  Address virt;
		  if (Jdb::peek_addr_task(entry, task, &virt) != -1)
		    {
		      if (!dump(virt, task, level+1))
			return NOTHING;
		      redraw = true;
		    }
		}
	      break;
	    case 'u': // disassemble using address the cursor points to
	      if (jdb_disasm_addr_task 
		  && level<=7 && dump_type == D_MODE)
		{
		  Address virt;
		  if (Jdb::peek_addr_task(entry, task, &virt) != -1)
		    {
		      Jdb::printf_statline("u[address=%08x task=%x] ", 
					    virt, task);
		      int c1 = getchar();
		      if (c1 != KEY_RETURN && c1 != ' ')
			{
			  Jdb::print_current_tid_statline();
			  putchar('u');
			  Jdb::execute_command("u", c1);
			  return NOTHING;
			}

		      if (!jdb_disasm_addr_task(virt, task, level+1))
			return NOTHING;
		      goto screen;
		    }
		}
	      break;

	    case 'e': // poke memory
	      if (dump_type == D_MODE)
	    	{
		  Mword mword;
		  if (Jdb::peek_mword_task(entry, task, &mword) != -1
		      && (!Jdb::is_adapter_memory(entry, task)
			  || show_adapter_memory))
		    {
		      Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
		      putstr("        ");
		      Jdb::printf_statline("edit <%08x> = %08x",
					   entry, mword);
		      Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
		      Mword new_mword;
		      if (!Jdb_input::get_mword(&new_mword, 8, 16))
			{
			  // abort, restore old value
			  Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
			  printf("%08x", mword);
			  break;
			}

		      Jdb::poke_mword_task(entry, task, new_mword);
		      goto screen;
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
Jdb_dump::dump_memory(Mword lines, char dump_type, Address entry, 
		      Task_num task, Mword elb, Mword epl)
{
  // prevent apic from getting confuse by invalid register accesses
  if (&ignore_invalid_apic_reg_access)
    ignore_invalid_apic_reg_access = 1;

  for (Mword i=0; i<lines; i++, entry += elb*epl)
    {
      Kconsole::console()->getchar_chance();

      printf("%0*x", sizeof(Address)*2, entry);
      Mword mapped  = Jdb::peek_task(entry, task) != -1;
      Mword ram     = !Jdb::is_adapter_memory(entry, task);


      // dump one line
      for (Mword x=0; x < epl; x++)
	{
	  putchar(x == 0 ? ':' : ' ');

	  if (mapped)
	    {
	      if (ram || show_adapter_memory)
		{
		  Mword mword;
		  Jdb::peek_mword_task(entry + x*elb, task, &mword);

		  if (dump_type==D_MODE)
		    {
		      if (mword == 0)
			printf("%*d", elb*2,  0);
		      else if (mword == (Mword)-1)
			printf("%*d", elb*2, -1);
		      else
			printf("%0*x", elb*2, mword);
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
		  Mword elc = dump_type==C_MODE ? elb : elb*2;
		  for (Mword u=0; u<elc; u++)
		    putchar('-');
		}
	    }
	  else // !mapped
	    {
	      Mword elc = dump_type==C_MODE ? elb : elb*2;
	      for (Mword u=0; u<elc; u++)
    		putchar('.');
	    }
	}

      // end of line
      if (dump_type==C_MODE)
	Jdb::clear_to_eol();

      putchar('\n');
    }

  if (&ignore_invalid_apic_reg_access)
    ignore_invalid_apic_reg_access = 0;
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
	  return dump(addr, Jdb::translate_task(addr, task), 0);
	}

      if (code != LEAVE)
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
      Cmd (0, "d", "dump", "%C",
	  "d[t<taskno>]<addr>\tdump memory of given/current task at <addr>",
	  &Jdb_input_task_addr::first_char),
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
  : Jdb_module("INFO")
{}

static Jdb_dump jdb_dump INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

int
jdb_dump_addr_task(Address addr, Task_num task, int level)
{ return Jdb_dump::dump(addr, task, level); }

