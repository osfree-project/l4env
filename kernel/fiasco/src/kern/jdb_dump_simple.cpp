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
private:
  static Address dump_address;
};

Address Jdb_dump::dump_address;

PUBLIC static
Jdb_module::Action_code
Jdb_dump::dump(Address virt, int level)
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
  Mword cols  = 8;
  Mword line_offs = epl*elb;
  Mword line_mask = line_offs-1;
  Mword line_page = Config::PAGE_SIZE / line_offs;
  int dump_type = D_MODE;

  if (level==0)
    Jdb::clear_screen();

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
  dump_memory(lines, dump_type, entry_base + absy*line_offs, elb, epl);

  for (bool redraw=false; ; )
    {
      entry = entry_base + (absy+addy)*line_offs + addx*elb;

      if (old_entry_set && (entry != old_entry))
	virt += entry-old_entry;

      old_entry     = entry;
      old_entry_set = true;

      Jdb::printf_statline("dump", (dump_type==D_MODE)
		        ? "e=edit u=disasm D=dump <Space>=mode <CR>=goto addr"
			: "<Space>=mode", "%c<%08x>", dump_type, virt);
      
      // mark next value
      Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
      int c = Jdb_core::getchar();

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

      if (!Jdb::std_cursor_key(c, cols, lines, max_absy, 
			       &absy, &addy, &addx, &redraw))
	{
	  switch(c)
	    {
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
			  low_addr  &= ~line_mask;
			  high_addr += 2*line_offs-1;
			  high_addr &= ~line_mask;

			  if (low_addr <= high_addr)
			    {
			      Mword lines = (high_addr-low_addr-1) / line_offs;
			      if (lines < 1)
				lines = 1;
			      // enable gzip console
			      Kconsole::console()->
				start_exclusive(Console::GZIP);
			      dump_memory(lines, D_MODE, low_addr, 
					  elb, epl);
			      Kconsole::console()->
				end_exclusive(Console::GZIP);
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
	      redraw = true;
	      break;
#if 0
	    case KEY_RETURN: // goto address under cursor
	      if (level<=7 && dump_type==D_MODE)
		{
		  Address virt = *((Address*)entry);
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
			  Jdb::printf_statline(0);
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
#endif
	    case 'e': // poke memory
	      if (dump_type == D_MODE)
	    	{
		  Mword mword = *(Mword*)entry;
		  if (1)
		    {
		      Jdb::cursor(addy+1, (elc+1)*addx+bol+1);
		      putstr("        ");
		      Jdb::printf_statline("dump", 0, "edit <%08x> = %08x",
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

		      *(Mword*)entry = new_mword;
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
		      Mword elb, Mword epl)
{
  for (Mword i=0; i<lines; i++, entry += elb*epl)
    {
      Kconsole::console()->getchar_chance();

      printf("%0*x", sizeof(Address)*2, entry);
      Mword const mapped  = 1;

      // dump one line
      for (Mword x=0; x < epl; x++)
	{
	  putchar(x == 0 ? ':' : ' ');

	  if (mapped)
	    {
	      Mword mword = *(Mword*)(entry+x*elb);;
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
}

PUBLIC
Jdb_module::Action_code
Jdb_dump::action(int cmd, void *&, char const *&, int &)
{
  if (cmd == 0)
    {
      return dump(dump_address, 0);
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_dump::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "d", "dump", "%x",
	  "d<addr>\tdump memory of current task at <addr>",
	  &dump_address },
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

