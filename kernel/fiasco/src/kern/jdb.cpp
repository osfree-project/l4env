INTERFACE:

#include "l4_types.h"

class Context;

class Jdb_entry_frame 
{
};

class Jdb
{
public:
  static Jdb_entry_frame *entry_frame;

public:
  static void enter_jdb(Jdb_entry_frame *e) asm ("enter_jdb");

  static int  set_prompt_color(char x);

  static void cursor_end_of_screen();
  static void cursor(int y, int x);
  static void cursor_home();
  static void clear_to_eol();
  static void clear_screen();
  static void screen_setscroll(int bkegin, int end);

  static Context *current_context();

private:
  static char show_statline;
  static char next_cmd;
  static char prompt_esc[];
  static L4_uid current_tid;

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

char   Jdb::next_cmd;			// next global command to execute
char   Jdb::show_statline = true;		// show status line on enter_kdebugger
char   Jdb::prompt_esc[32] = "\033[32;1m"; // light green
L4_uid Jdb::current_tid;		// tid of current thread
Jdb_entry_frame *Jdb::entry_frame;

enum { LOGO = 5 };

// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
PRIVATE static
void Jdb::printf_statline(const char *format, ...)
{
  cursor(Jdb_screen::height(), 1);
  putstr(prompt_esc);

  if (!format)
    {
      if (!current_tid.is_invalid())
	{
	  printf("(%x.%02x) ",
	      current_tid.task(), current_tid.lthread());
	}
    }
  putstr("Jdb: \033[0m");

  if (format)
    {
      va_list list;
      va_start(list, format);
      vprintf(format, list);
      va_end(list);
    }
  clear_to_eol();
}


PRIVATE static 
bool Jdb::is_toplevel_cmd(char c)
{
  char cm[] = {c,0};
  Jdb_core::Cmd cmd = Jdb_core::has_cmd( cm );
  if(cmd.cmd) {
    return true;
  }
  return false;
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
	      next_cmd = 0;
	    }
	  else   
	    c=getchar();
	} while (c<' ' && c!=13);

      printf("\033[K%c", c);
	
      char _cmd[] = {c,0};
      Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);
      if(cmd.cmd) {
	return Jdb_core::exec_cmd( cmd );
      }
    }
}

static Proc::Status jdb_irq_state;

// save pic state and mask all interupts
PRIVATE static 
void Jdb::open_debug_console()
{
  jdb_irq_state = Proc::cli_save();
  // save and clear state of debug control registers
  // jdb_bp::save_state();
  // save_disable_irqs();
}


PRIVATE static 
void Jdb::close_debug_console()
{
  //com_cons_enable_receive_interrupt();
  // restore_irqs();
  // jdb_bp::restore_state();
  Proc::sti_restore(jdb_irq_state);
}


IMPLEMENT void
Jdb::enter_jdb(Jdb_entry_frame *e) 
{
  entry_frame = e;
  volatile bool really_break = true;
  static char error_buffer[81];
  // don't enter kdebug if user pressed a non-debug command key
  int c = Kconsole::console()->getchar(false);
  if (   (c != -1)		/* valid input */
	 && (c != 0x20)	/* <SPACE> */
	 && (c != 0x1b)	/* <ESC> */
	 && (!is_toplevel_cmd(c)))
    return;
  
  // console_buffer()->disable();

  // disable all interrupts
  open_debug_console();
  show_statline = true;

  // clear error message
  *error_buffer = '\0';
 
  if (really_break) 
    {
      //get_current(ts);

      do 
	{
	  screen_setscroll(1, Jdb_screen::height());
	  if (show_statline) 
	    {
	      cursor(Jdb_screen::height(), 1);
	      putstr(prompt_esc);
	      printf("\n"
		     "    "
		     "---------------------------------------------------------"
		     "PC: %08x\033[0m       \n", 
		     entry_frame->pc);
	      cursor(Jdb_screen::height()-1, 6);
	      printf("%s", error_buffer);
	      show_statline = false;
	    }

	  printf_statline(0);

	} while (execute_command());

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
void Jdb::cursor(int y, int x)
{
  printf("\033[%d;%dH", y, x);
}

IMPLEMENT
void Jdb::clear_to_eol()
{
  putstr("\033[K");
}

IMPLEMENT
void Jdb::cursor_home()
{
  putstr("\033[H");
}

IMPLEMENT
void Jdb::clear_screen()
{
  printf("\033[%d;1H", Jdb_screen::height());
  for (unsigned i=0; i<Jdb_screen::height(); i++)
    {
      putchar('\n');
      clear_to_eol();
    }
  cursor_home();
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

IMPLEMENT
int Jdb::set_prompt_color(char x)
{
  unsigned pc = 32;
  unsigned ph = 0;
 
  switch(x) 
    {
    case 'N':
      ph = 1;
    case 'n':
      pc = 30;
      break;

    case 'R':
      ph = 1;
    case 'r':
      pc = 31;
      break;

    case 'G':
      ph = 1;
    case 'g':
      pc = 32;
      break;
      
    case 'Y':
      ph = 1;
    case 'y':
      pc = 33;
      break;

    case 'B':
      ph = 1;
    case 'b':
      pc = 34;
      break;

    case 'M':
      ph = 1;
    case 'm':
      pc = 35;
      break;

    case 'C':
      ph = 1;
    case 'c':
      pc = 36;
      break;

    case 'W':
      ph = 1;
    case 'w':
      pc = 37;
      break;
      
    default:
      return 0;

    }

  if(ph>0)
    snprintf(prompt_esc,31,"\033[%d;%dm",pc,ph);
  else
    snprintf(prompt_esc,31,"\033[%dm",pc);

  return 1;

}

