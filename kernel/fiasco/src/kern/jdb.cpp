INTERFACE:

#include "l4_types.h"
#include "jdb_core.h"
#include "jdb_handler_queue.h"

class Context;
class Thread;

class Jdb_entry_frame 
{
public:
  bool from_user() const;
  Address ip() const;
};

class Jdb : public Jdb_core
{
public:
  static Jdb_entry_frame *get_entry_frame();
  static Jdb_entry_frame *entry_frame;

public:
  
  static void enter_jdb(Jdb_entry_frame *e, char const *msg) asm ("enter_jdb");
  static void cursor_end_of_screen();
  static void cursor_home();
  static void screen_setscroll(int bkegin, int end);
  static Context *current_context();
  static void printf_statline(const char *prompt, const char *help,
	  		      const char *format, ...);
  /*  __attribute__((format(printf, 3, 4)));*/
  static void set_next_cmd( char c );


  // formating print functions
  static int write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign);
  static int write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign);
  static int write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign);
  
  static int std_cursor_key(int c, Mword cols, Mword lines, Mword max_absy, 
                            Mword *absy, Mword *addy, Mword *addx, 
                            bool *redraw);
private:
  static char show_statline;
  static char next_cmd;
  static char error_buffer[57];
  
public:
  static Jdb_handler_queue jdb_enter;
  static Jdb_handler_queue jdb_leave;

  static void (*alt_entry)(Jdb_entry_frame *e, char const *msg);
  
  static char  esc_iret[];
  static char  esc_emph[];
  static char  esc_emph2[];
  
  static bool was_input_error;
  static Thread  *current_active;
};

IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include <simpleio.h>

#include "config.h"
#include "jdb_core.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "processor.h"
#include "static_init.h"
#include "keycodes.h"
#include "virq.h"

Jdb_handler_queue Jdb::jdb_enter;
Jdb_handler_queue Jdb::jdb_leave;
void (*Jdb::alt_entry)(Jdb_entry_frame *e, char const *msg);
char Jdb::error_buffer[57];
char Jdb::next_cmd;			// next global command to execute
char Jdb::show_statline = true;		// show status line on enter_kdebugger
Jdb_entry_frame *Jdb::entry_frame;
Thread *Jdb::current_active;		// current running thread
bool Jdb::was_input_error;


IMPLEMENT inline
Jdb_entry_frame *Jdb::get_entry_frame()
{
  return entry_frame;
}

  IMPLEMENT inline
void Jdb::set_next_cmd( char cmd )
{
  next_cmd = cmd;
}

PUBLIC
static void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), 6);
  clear_to_eol();

  was_input_error = true;
}

// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
IMPLEMENT
void Jdb::printf_statline(const char *prompt, const char *help,
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
  if (format)/*format && (format[0] != '_' || format[1] != '\0'))*/
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
Thread *
Jdb::get_current_active()
{
  return current_active;
}


PUBLIC static 
bool Jdb::is_toplevel_cmd(char c)
{
  char cm[] = {c,0};
  Jdb_core::Cmd cmd = Jdb_core::has_cmd( cm );
  if(cmd.cmd) {
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


PRIVATE static 
int Jdb::execute_command()
{

  for (;;) 
    {
      int c;
      do 
	{
	  if (next_cmd) 
	    {
	      c = next_cmd;
	      set_next_cmd(0);
	    }
	  else   
	    c=getchar();
	} 
      while (c<' ' && c!=KEY_RETURN);

      if (c == KEY_F1)
	c = 'h';
      printf("\033[K%c", c);

      char _cmd[] = {c,0};
      Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);
      if(cmd.cmd) 
	{
	  return Jdb_core::exec_cmd( cmd );
        }
      
      if (c == KEY_RETURN)
	{
	  show_statline = true;
  	  return 1;
	}
    }
}

static Proc::Status jdb_irq_state;

// save pic state and mask all interupts
PRIVATE static 
void Jdb::open_debug_console()
{
  jdb_irq_state = Proc::cli_save();
  jdb_enter.execute();
  // save and clear state of debug control registers
  // jdb_bp::save_state();
  // save_disable_irqs();
}


PRIVATE static 
void Jdb::close_debug_console()
{
  // eat up input from console
  while (Kconsole::console()->getchar(false)!=-1)
    ;
  jdb_leave.execute();
  Proc::sti_restore(jdb_irq_state);
}

IMPLEMENT void
Jdb::enter_jdb(Jdb_entry_frame *e, char const *msg) 
{
  entry_frame = e;

  if (alt_entry)
    {
      alt_entry(e, msg);
      return;
    }

  volatile bool really_break = true;

  // don't enter kdebug if user pressed a non-debug command key
  int c = Kconsole::console()->getchar(false);
  if (   (c != -1)		/* valid input */
	 && (c != 0x20)	/* <SPACE> */
	 && (c != 0x1b)	/* <ESC> */
	 && (!is_toplevel_cmd(c)))
    return;
  
  open_debug_console();
  show_statline = true;
  if (msg)
    snprintf(error_buffer, sizeof(error_buffer)-1, "%s",msg);
  else
    error_buffer[0] = 0;
 
  if (really_break) 
    {
      update_prompt();

      do 
	{
	  screen_setscroll(1, Jdb_screen::height());
	  if (show_statline) 
	    {
	      cursor(Jdb_screen::height(), 1);
	      putstr(esc_prompt);
	      printf("\n  --\033[0m");
	      printf("%s",error_buffer);
	      putstr(esc_prompt);
	      for (int x=57-strlen(error_buffer); x>0; --x)
		putchar('-');
	      printf("PC: %08x\033[0m       \n", entry_frame->pc);
	      show_statline = false;
	    }

	  printf_statline(0, 0, 0);

	} while (execute_command());

      if (alt_entry)
	{
	  alt_entry(e, msg);
	  return;
	}

      // scroll one line
      putchar('\n');
      
      // reset scrolling region of serial terminal

      screen_setscroll(1,127);
      
      // goto end of screen
      cursor_end_of_screen();
      //console_buffer()->enable();
    }
 
  // reenable interrupts
  close_debug_console();

  if(Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::uart()->enable_rcv_irq();

  return;
}

PUBLIC
static int
Jdb::getchar(void)
{
  return Kconsole::console()->getchar();
}

IMPLEMENT
void Jdb::cursor_home()
{
  putstr("\033[H");
}

IMPLEMENT
void Jdb::screen_setscroll(int begin, int end)
{
  printf("\033[%d;%dr",begin,end);
}

IMPLEMENT
void Jdb::cursor_end_of_screen()
{
  putstr("\033[127;1H");
}

//-------- pretty print functions ------------------------------
IMPLEMENT
int Jdb::write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = (ns < 0) ? -ns : ns;
  int len = 0;

  if (uns >= 1000000000000ULL)
    {
      strncpy(buf, ">99     s ", maxlen);
      len = 10;
    }
  else
    {
      if (sign)
	{
	  *buf++ = (ns < 0) ? '-' : (ns == 0) ? ' ' : '+';
	  maxlen--;
	  len = 1;
	}
      for (; maxlen>11; maxlen--, len++)
	*buf++ = ' ';
      if (uns >= 1000000000)
	{
	  Mword _s  = uns / 1000000000ULL;
	  Mword _ms = (uns - 1000000000ULL * (Unsigned64)_s) / 1000000ULL;
	  len += snprintf(buf, maxlen, "%3lu.%03lu s ", _s, _ms);
	}
      else if (uns >= 1000000)
	{
	  Mword _ms = uns / 1000000UL;
	  Mword _us = (uns - 1000000UL * (Unsigned64)_ms) / 1000UL;
	  len += snprintf(buf, maxlen, "%3lu.%03lu ms", _ms, _us);
	}
      else if (uns == 0)
	{
	  strncpy(buf, "  0       ", maxlen);
	  len += 10;
	}
      else
	{
	  Console* gzip = Kconsole::console()->find_console(Console::GZIP);
	  Mword _us = uns / 1000UL;
	  Mword _ns = (uns - 1000UL * (Unsigned64)_us);
	  len += snprintf(buf, maxlen, "%3lu.%03lu %c ", _us, _ns, 
			  gzip && gzip->state() & Console::OUTENABLED
				? '\265' : Config::char_micro);
	}
    }

  return len;
}

IMPLEMENT
int Jdb::write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign)
{
  // display 40 bits
  Unsigned64 xu = (x < 0) ? -x : x;

  return snprintf(buf, maxlen, "%s%02lx%08lx",
		  sign ? (x < 0) ? "-" : (x == 0) ? " " : "+" : "",
		  (Mword)((xu >> 32) & 0xff), (Mword)xu);
}

IMPLEMENT
int Jdb::write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign)
{
  Unsigned64 xu = (x < 0) ? -x : x;

  // display decimal
  if (xu > 0xFFFFFFFFULL)
    {
      strncpy(buf, " >>       ", maxlen);
      return 10;
    }

  return snprintf(buf, maxlen, "%s%10lu",
		  sign ? x < 0 ? "-" 
			       : (x == 0) ? " " 
					  : "+"  
		       : "", (Mword)xu);
}

/// handling of standard cursor keys (Up/Down/PgUp/PgDn)
IMPLEMENT
int Jdb::std_cursor_key(int c, Mword cols, Mword lines, Mword max_absy, Mword *absy,
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
	  else if (*addy < lines - 1)
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

//---------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

char Jdb::esc_emph[]     = "\033[33m";
char Jdb::esc_emph2[]    = "\033[32m";
char Jdb::esc_iret[]     = "\033[36m";

