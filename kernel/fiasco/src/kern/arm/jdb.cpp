INTERFACE:

#include "l4_types.h"

struct Jdb_regs
{
  unsigned r0, r1, r2, r3;
  unsigned r4, r5, r6, r7;
  unsigned r8, r9, r10, r11;
  unsigned r12, r13, r14, r15;
};

class Trap_state;

class Jdb
{
public:
  static void init();
  static bool enter_jdb() asm ("enter_jdb");

private:
  static int main_loop( Trap_state * );

  static char show_statline;
  static char next_cmd;
  static char prompt_esc[];
  static L4_uid current_tid;		// tid of current thread

};

IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "jdb_core.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "static_init.h"

enum {
  LAST_LINE =	24,
  LOGO =	5,
};

char Jdb::next_cmd = 0;			// next global command to execute
char Jdb::show_statline = true;		// show status line on enter_kdebugger
L4_uid Jdb::current_tid;		// tid of current thread
char Jdb::prompt_esc[32] = "\033[32;1m"; // light green

static void Jdb::cursor(int y, int x)
{
  printf("\033[%d;%dH",y+1,x+1);
}

static void clear_to_eol()
{
  putstr("\033[K");
}

static void home()
{
  putstr("\033[H");
}

static void fancy_clear_screen()
{
  putstr("\033[24;1H");
  for (int i=0; i<25; i++)
    {
      putchar('\n');
      clear_to_eol();
    }
  home();
}

static void serial_setscroll(int begin, int end)
{
  printf("\033[%d;%dr",begin,end);
}

static void serial_end_of_screen()
{
  putstr("\033[127;1H");
}


#if 0
PRIVATE static
void Jdb::prompt()
{
  if(short_mode) {
    printf("\nJDB: ");
  } else {
    printf("\njdb/%s:",act_path);
  }
}
#endif

PUBLIC static 
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




// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
PRIVATE static
void Jdb::printf_statline(const char *format, ...)
{
  cursor(LAST_LINE, 0);
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
int Jdb::execute_command(Trap_state *ts)
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

// save pic state and mask all interupts
PRIVATE static 
void Jdb::open_debug_console()
{
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
}


PUBLIC static
int
Jdb::enter_kdebugger(Trap_state *ts)
{
  //Cpu::disable_lbr();

  volatile bool really_break = true;

  static char error_buffer[81];

  // don't enter kdebug if user pressed a non-debug command key
  int c = Kconsole::console()->getchar(false);
  if (   (c != -1)		/* valid input */
	 && (c != 0x20)	/* <SPACE> */
	 && (c != 0x1b)	/* <ESC> */
	 && (!is_toplevel_cmd(c)))
    return 0;
  
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
	  serial_setscroll(1, 25);
	  if (show_statline) 
	    {
	      cursor(24, 0);
	      putstr(prompt_esc);
	      printf("\n"
		     "    "
		     "--------------------------------------------------------"
		     "EIP: %08x\033[0m       \n", 
		     0);
	      cursor(23, 6);
	      printf("%s", error_buffer);
	      show_statline = false;
	    }

	  printf_statline(0);

	} while (execute_command(ts));

      // scroll one line
      putchar('\n');
      
      // reset scrolling region of serial terminal

      serial_setscroll(1,127);
      
      // goto end of screen
      serial_end_of_screen();
      //console_buffer()->enable();
    }
 
  // reenable interrupts
  close_debug_console();

  if(Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::uart()->enable_rcv_irq();

  return 0;
}


IMPLEMENT
bool Jdb::enter_jdb() 
{
  asm volatile ( " sub    sp, sp, %0     \n" 
		 " stmia  sp, {r0-r14}   \n"
		 " str    lr, [sp, %0-4] \n"
		 : : "I"(sizeof(Jdb_regs)) : "sp" );

  register unsigned sp asm ("sp");
  //  regs = (Jdb_regs*)sp;


  enter_kdebugger(0);


  asm volatile ( " add    sp, sp, %0     \n"
		 : : "I"(sizeof(Jdb_regs)) : "sp" );
  return true;
}

STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);

IMPLEMENT
void Jdb::init()
{
}




