INTERFACE[ia32,ux]:

#include "jdb_core.h"
#include "jdb_handler_queue.h"
#include "trap_state.h"

class Space;
class Thread;

class Jdb_entry_frame : public Trap_state
{};

EXTENSION class Jdb : public Jdb_core
{
public:
  static void    printf_statline (const char *prompt, const char *help,
				  const char *format, ...)
    __attribute__((format(printf, 3, 4)));

  static int     getchar(void);
  static int     was_last_cmd(void);
  static int     get_next_cmd(void);
  static void    set_next_cmd(char c);
  static void    flush_next_cmd(void);
  static bool    is_toplevel_cmd(char c);

  static Jdb_handler_queue jdb_enter;
  static Jdb_handler_queue jdb_leave;

private:
  static Thread  *current_active;
  static char    last_cmd;
  static char    next_cmd;
  static Trap_state::Handler nested_trap_handler FIASCO_FASTCALL;
  static Jdb_entry_frame *entry_frame;
};


IMPLEMENTATION[ia32,ux]:

#include "config.h"
#include "div32.h"
#include "kernel_console.h"
#include "paging.h"

Trap_state::Handler Jdb::nested_trap_handler FIASCO_FASTCALL;
Jdb_handler_queue Jdb::jdb_enter;
Jdb_handler_queue Jdb::jdb_leave;
Thread           *Jdb::current_active;		// current running thread
char              Jdb::last_cmd;
char              Jdb::next_cmd;
Jdb_entry_frame  *Jdb::entry_frame;


// handling of standard cursor keys (Up/Down/PgUp/PgDn)
PUBLIC static
int
Jdb::std_cursor_key(int c, Mword cols, Mword lines, Mword max_absy, Mword *absy,
		    Mword *addy, Mword *addx, bool *redraw)
{
  switch (c)
    {
    case KEY_CURSOR_LEFT:
      if (addx)
	{
	  if (*addx > 0)
	    (*addx)--;
	  else if (*addy > 0)
	    {
	      (*addy)--;
	      *addx = cols - 1;
	    }
	  else if (*absy > 0)
	    {
	      (*absy)--;
	      *addx = cols - 1;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_RIGHT:
      if (addx)
	{   
	  if (*addx < cols - 1)
	    (*addx)++;
	  else if (*addy < lines-1)
	    {
	      (*addy)++; 
	      *addx = 0;
	    }
	  else if (*absy < max_absy)
	    {
	      (*absy)++;
	      *addx = 0;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_UP:
      if (*addy > 0)
	(*addy)--;
      else if (*absy > 0)
	{
	  (*absy)--;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_DOWN:
      if (*addy < lines-1)
	(*addy)++;
      else if (*absy < max_absy)
	{
	  (*absy)++;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_HOME:
      *addy = 0;
      if (addx)
	*addx = 0;
      if (*absy > 0)
	{
	  *absy = 0;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_END:
      *addy = lines-1;
      if (addx)
	*addx = cols - 1;
      if (*absy < max_absy)
	{
	  *absy = max_absy;
	  *redraw = true;
	}
      break;
    case KEY_PAGE_UP:
      if (*absy >= lines)
	{
	  *absy -= lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy > 0)
	    {
	      *absy = 0;
	      *redraw = true;
	    }
	  else if (*addy > 0)
	    *addy = 0;
	  else if (addx)
	    *addx = 0;
	}
      break;
    case KEY_PAGE_DOWN:
      if (*absy+lines-1 < max_absy)
	{
	  *absy += lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy < max_absy)
	    {
	      *absy = max_absy;
	      *redraw = true;
	    }
	  else if (*addy < lines-1)
      	    *addy = lines-1;
	  else if (addx)
	    *addx = cols - 1;
	}
      break;
    default:
      return 0;
    }

  return 1;
}

IMPLEMENT inline
int
Jdb::was_last_cmd()
{
  return last_cmd;
}

IMPLEMENT inline
int
Jdb::get_next_cmd()
{
  return next_cmd;
}

IMPLEMENT inline
void
Jdb::set_next_cmd(char c)
{
  next_cmd = c;
}

IMPLEMENT
int
Jdb::getchar(void)
{
  return Kconsole::console()->getchar();
}

IMPLEMENT
bool
Jdb::is_toplevel_cmd(char c)
{
  char cm[] = { c, 0 };
  Jdb_core::Cmd cmd = Jdb_core::has_cmd (cm);

  if(cmd.cmd || (0 != strchr(toplevel_cmds, c)))
    {
      set_next_cmd(c);
      return true;
    }

  return false;
}


PUBLIC static
int
Jdb::execute_command(const char *s, int first_char = -1)
{
  Jdb_core::Cmd cmd = Jdb_core::has_cmd(s);

  if (cmd.cmd)
    return Jdb_core::exec_cmd(cmd, first_char) == 2 ? 1 : 0;

  return 0;
}

// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
IMPLEMENT
void
Jdb::printf_statline(const char *prompt, const char *help,
		     const char *format, ...)
{
  cursor(Jdb_screen::height(), 1);
  if (help)
    {
      printf("%79s", help);
      cursor(Jdb_screen::height(), 1);
    }
  prompt_start();
  if (prompt)
    {
      putstr(prompt);
      putstr(": ");
    }
  else
    Jdb::prompt();
  prompt_end();
  // work around for ealier gccs complaining about "empty format strings"
  if (format && (format[0] != '_' || format[1] != '\0'))
    {
      va_list list;
      va_start(list, format);
      vprintf(format, list);
      va_end(list);
    }
  if (!help)
    clear_to_eol();
}

PUBLIC static
void
Jdb::write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = (ns < 0) ? -ns : ns;
  Unsigned32 ums = div32(uns, 1000000);

  if (ums >= 3600000000U)
    {
      snprintf(buf, maxlen, ">999 h ");
      return;
    }

  if (sign)
    {
      *buf++ = (ns < 0) ? '-' : (ns == 0) ? ' ' : '+';
      maxlen--;
    }

  if (ums >= 60000000)
    {
      // 1000min...999h
      Mword _h  = ums / 3600000;
      Mword _m  = (ums - 3600000 * _h) / 60000;
      snprintf(buf, maxlen, "%3lu:%02lu h  ", _h, _m);
      return;
    }

  if (ums >= 1000000)
    {
      // 1000s...999min
      Mword _m  = ums / 60000;
      Mword _s  = (ums - 60000 * _m) / 1000;
      snprintf(buf, maxlen, "%3lu:%02lu M  ", _m, _s);
      return;
    }

  if (ums >= 1000)
    {
      // 1...1000s
      Mword _s  = ums / 1000;
      Mword _ms = ums - 1000 * _s;
      snprintf(buf, maxlen, "%3lu.%03lu s ", _s, _ms);
      return;
    }

  if (uns >= 1000000)
    {
      // 1...1000ms
      Mword _ms = (Mword)uns / 1000000;
      Mword _us = ((Mword)uns - 1000000 * _ms) / 1000;
      snprintf(buf, maxlen, "%3lu.%03lu ms", _ms, _us);
      return;
    }

  if (uns == 0)
    {
      snprintf(buf, maxlen, "  0       ");
      return;
    }

  // 1...1000µs
  Console* gzip = Kconsole::console()->find_console(Console::GZIP);
  Mword _us = (Mword)uns / 1000;
  Mword _ns = (Mword)uns - 1000 * _us;
  snprintf(buf, maxlen, "%3lu.%03lu %c ", _us, _ns,
	       gzip && gzip->state() & Console::OUTENABLED
	       ? '\265' : Config::char_micro);
}

PUBLIC static
void
Jdb::write_tsc_s(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  Unsigned32 ums = div32(uns, 1000000);

  if (tsc < 0)
    uns = -uns;

  if (ums >= 3600000000U)
    {
      snprintf(buf, maxlen, ">999 h ");
      return;
    }

  if (sign)
    {
      *buf++ = (tsc < 0) ? '-' : (tsc == 0) ? ' ' : '+';
      maxlen--;
    }

  if (ums >= 60000000)
    {
      // 1:00...999:00 h
      Mword _h  = ums / 3600000;
      Mword _m  = (ums - 3600000 * _h) / 60000;
      snprintf(buf, maxlen, "%3lu:%02lu     h ", _h, _m);
      return;
    }

  if (ums >= 1000000)
    {
      // 1:00...999:00 min
      Mword _m  = ums / 60000;
      Mword _s  = (ums - 60000 * _m) / 1000;
      snprintf(buf, maxlen, "%3lu:%02lu    min", _m, _s);
      return;
    }

  if (ums >= 1000)
    {
      // 1.000000...999.000000 s
      Mword _s  = ums / 1000;
      Mword _us = div32(uns, 1000) - 1000000 * _s;
      snprintf(buf, maxlen, "%3lu.%06lu s ", _s, _us);
      return;
    }

  if (uns == 0)
    {
      snprintf(buf, maxlen, "  0          ");
      return;
    }

  // 1.000000...999.000000 ms
  Mword _ms = ums;
  Mword _ns = ((Mword)uns - 1000000 * _ms);
  snprintf(buf, maxlen, "%3lu.%06lu ms", _ms, _ns);
}

PUBLIC static
void
Jdb::write_tsc(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  Unsigned64 ns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  if (tsc < 0)
    ns = -ns;
  write_ll_ns(ns, buf, maxlen, sign);
}

PUBLIC static
void
Jdb::write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign)
{
  // display no more than 40 bits
  Unsigned64 xu = (x < 0) ? -x : x;

  if (sign)
    snprintf(buf, maxlen, "%s%03lx" L4_PTR_FMT,
			  (x < 0) ? "-" : (x == 0) ? " " : "+",
			  (Mword)((xu >> 32) & 0xfff), (Mword)xu);
  else
    snprintf(buf, maxlen, "%04lx" L4_PTR_FMT,
			  (Mword)((xu >> 32) & 0xffff), (Mword)xu);
}

PUBLIC static
void
Jdb::write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign)
{
  Unsigned64 xu = (x < 0) ? -x : x;

  // display no more than 11 digits
  if (xu >= 100000000000ULL)
    {
      snprintf(buf, maxlen, "%12s", ">= 10^11");
      return;
    }

  if (sign && x != 0)
    snprintf(buf, maxlen, "%+12lld", x);
  else
    snprintf(buf, maxlen, "%12llu", xu);
}

PUBLIC static inline
Thread*
Jdb::get_current_active()
{
  return current_active;
}

// Interprete str as non interactive commands for Jdb. We allow mostly 
// non-interactive commands here (e.g. we don't allow d, t, l, u commands)
PRIVATE static
int
Jdb::execute_command_ni(Unsigned8 const *str, int len=0)
{
  Thread *t = get_thread();

  Push_console::push(str, len, t == reinterpret_cast<Thread*>(Mem_layout::Tcbs)
			       ? 0 : t->space());

  // prevent output of sequences
  Kconsole::console()->change_state(0, 0, ~Console::OUTENABLED, 0);

  for (;;)
    {
      int c = getchar();
     
      was_input_error = true;
      if (0 != strchr(non_interactive_cmds, c))
	{
	  char _cmd[] = {c, 0};
	  Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

	  if (cmd.cmd)
	    {
	      if (Jdb_core::exec_cmd (cmd) != 3)
    		was_input_error = false;
	    }
	}

      if (c == KEY_RETURN || c == ' ' || was_input_error)
	{
	  Push_console::flush();
	  // re-enable all consoles but GZIP
	  Kconsole::console()->change_state(0, Console::GZIP,
					    ~0U, Console::OUTENABLED);
	  return c == KEY_RETURN || c == ' ';
	}
    }
}


PUBLIC static inline
Jdb_entry_frame*
Jdb::get_entry_frame()
{
  return entry_frame;
}


PUBLIC inline
Address_type
Jdb_entry_frame::from_user()
{
  return cs & 3 ? ADDR_USER : ADDR_KERNEL;
}

PUBLIC inline
Address
Jdb_entry_frame::get_ksp()
{
  return (Address)&esp;
}

PUBLIC inline
Address
Jdb_entry_frame::_get_esp()
{
  return from_user() ? esp : get_ksp();
}

PUBLIC inline
Mword
Jdb_entry_frame::param()
{
  return eax;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ia32]:

PUBLIC inline NEEDS["cpu.h"]
Mword
Jdb_entry_frame::_get_ss()
{
  return from_user() ? ss : Cpu::get_ss();
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ux]:

PUBLIC
Mword
Jdb_entry_frame::_get_ss()
{
  return ss;
}
