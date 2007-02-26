INTERFACE:

#include "l4_types.h"
#include "pic.h"
#include "config.h"

class trap_state;
class Thread;

class Jdb
{
public:

  static void init();
  static Pic::Status pic_status;

private:
  Jdb();			// default constructors are undefined
  Jdb(const Jdb&);

  typedef struct 
    {
      char const *name;
      char error_code;
    } traps_t;
  
  static bool _connected;
  static bool was_error;

  static Thread *current_present, *current_ready, *current_active;
  static L4_uid current_tid;
  static unsigned specified_task;
  
  static char *bp_mode_names[4];
  static traps_t const traps[20];
  static const char * const reg_names[];

  static Mword bp_base;
  static Mword gdb_trap_eip;
  static Mword gdb_trap_cr2;
  static Mword gdb_trap_no;
  static Mword db7;

  static int (*nested_trap_handler)(trap_state *state);
  static int  next_inputchar;
 
  static char last_cmd;
  static char next_cmd;
  static char single_step_cmd;
  static char hide_statline;
  static char use_nested;
  static char auto_tcb;
  static char next_auto_tcb;
  static char lbr_active;
  
  static const char *toplevel_cmds;

  typedef enum
    {
      SS_NONE=0, SS_BRANCH, SS_RETURN
    } Step_state;

  typedef enum
    {
      s_unknown, s_ipc, s_pagefault, s_fputrap, 
      s_interrupt, s_timer_interrupt, s_slowtrap
    } Guessed_thread_state;

  static Step_state ss_state;
  static int ss_level;
  
  static bool code_ret, code_call, code_bra, code_int;

  static const char *debug_ctrl_str;
  static int        debug_ctrl_len;

  static char show_intel_syntax;
  static char show_lines;
  static char show_phys_mem;
  static char test_msr;

  static char gz_init_done;
  static char dump_gzip;
  static unsigned char vga_crtc_idx_val;
  static unsigned int  vga_crtc_idx;
  static Mword show_tb_nr;
  static Mword show_tb_refy;
  static Mword show_tb_absy;
  static char prompt_esc[];

  static const unsigned gz_heap_pages = 34*4096/Config::PAGE_SIZE; 
};

#define jdb_enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii \""text "\"	\n\t"\
    "1:			\n\t"\
    "nop;nop;nop	\n\t"\
    )



IMPLEMENTATION[ia32]:

#include <flux/gdb.h>
#include <flux/gdb_serial.h>
#include <flux/x86/trap.h>
#include <flux/x86/base_trap.h>

#include <flux/x86/base_idt.h>
#include <flux/x86/base_paging.h>
#include <flux/x86/tss.h>

#include <alloca.h>
#include <cstring>
#include <csetjmp>
//#include <unistd.h>
#include <cstdarg>
#include <climits>
#include <cstdlib>
#include <cstdio>

#include "initcalls.h"
#include "io.h"
#include "kernel_console.h"
#include "boot_info.h"
#include "entry_frame.h"
#include "cpu.h"
#include "thread.h" 
#include "thread_state.h" 
#include "region.h"
#include "uart.h"
#include "kernel_uart.h"
#include "virq.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "config.h"
#include "map_util.h"
#include "mapdb.h"
#include "mapdb_i.h"
#include "console.h"
#include "irq.h"
#include "timer.h"
#include "apic.h"
#include "checksum.h"
#include "regdefs.h"
#include "pic.h"

#include "console_buffer.h"

#include "jdb.h"
#include "jdb_trace.h"
#include "jdb_bp.h"
#include "jdb_tbuf.h"
#include "jdb_tbuf_init.h"
#include "jdb_tbuf_output.h"
#include "jdb_symbol.h"
#include "jdb_lines.h"
#include "jdb_thread_list.h"
#include "jdb_perf_cnt.h"

#include "processor.h"
#include "static_init.h"
#include "watchdog.h"
#include "regdefs.h"
#include "terminate.h"

#include "jdb_core.h"

#define SHOW_TRACEBUFFER_START	2
#define SHOW_TRACEBUFFER_LINES	21


// keystrokes
#define KEY_BACKSPACE	 0x08
#define KEY_TAB          0x09
#define KEY_ESC		 0x1b
#define KEY_RETURN	 0x0d
#define KEY_CURSOR_UP	 0x38
#define KEY_CURSOR_DOWN	 0x32
#define KEY_CURSOR_LEFT	 0x34
#define KEY_CURSOR_RIGHT 0x36
#define KEY_CURSOR_HOME	 0x37
#define KEY_CURSOR_END	 0x31
#define KEY_PAGE_UP	 0x39
#define KEY_PAGE_DOWN	 0x33

#define DEBREG_ACCESS	(1 << 13)
#define SINGLE_STEP	(1 << 14)

#define HI_MASK		0x0fffffff
#define PAGE_INVALID	0xffffffff

#define LAST_LINE	24
#define LOGO		5

// dump_address_space
#define B_MODE		'b'	// byte
#define C_MODE		'c'	// char
#define D_MODE		'd'	// dword
#define PAGE_DIR	'e'	// page directory
#define PAGE_TAB	'f'	// page table

// tracebuffer
#define INDEX_MODE	0
#define DELTA_TSC_MODE	1
#define REF_TSC_MODE 	2
#define START_TSC_MODE 	3
#define DELTA_PMC_MODE	4
#define REF_PMC_MODE	5

#define PCI_CONFIG_ADDR	0xcf8
#define PCI_CONFIG_DATA	0xcfc

// if !VERBOSE we are executing commands of an enter_kdebugger("*#") sequence
#define VERBOSE		(debug_ctrl_str == 0)

int  Jdb::next_inputchar = -1;
char Jdb::last_cmd;			// last executed global command
char Jdb::next_cmd;			// next global command to execute
char Jdb::single_step_cmd;		// explicit single_step command
char Jdb::hide_statline;		// show status line on enter_kdebugger
char Jdb::use_nested;			// switched to gdb
char Jdb::auto_tcb;			// show_tcb on single step
char Jdb::next_auto_tcb;		// next command ist auto show tcb
char Jdb::lbr_active;			// last branch recording active

// holds all commands executable in top level (regardless of current mode)
const char *Jdb::toplevel_cmds = "bdghijklmoprstuxABEIKLMOPSTUV?_^";

Mword Jdb::bp_base;			// base for breakpoints
Mword Jdb::gdb_trap_eip;		// eip on last trap while in Jdb
Mword Jdb::gdb_trap_cr2;		// cr2 on last trap while in Jdb
Mword Jdb::gdb_trap_no;			// trapno on last trap while in Jdb
Mword Jdb::db7;				// debug state when entered Jdb

Jdb::Step_state Jdb::ss_state = SS_NONE; // special single step state
int Jdb::ss_level;			// current call level
  
bool Jdb::code_ret;			// current instruction is ret/iret
bool Jdb::code_call;			// current instruction is call
bool Jdb::code_bra;			// current instruction is jmp/jxx
bool Jdb::code_int;			// current instruction is int x

Thread *Jdb::current_present;		// current present thread
Thread *Jdb::current_ready;		// current ready thread
Thread *Jdb::current_active;		// current running thread

L4_uid Jdb::current_tid;		// tid of current thread
unsigned Jdb::specified_task;		// specified task

int (*Jdb::nested_trap_handler)(trap_state *state);

bool Jdb::_connected;			// Jdb::init() was done
bool Jdb::was_error;			// error in command sequence

const char *Jdb::debug_ctrl_str;	// string+length for remote control of
int         Jdb::debug_ctrl_len;	// Jdb via enter_kdebugger("*#");

char Jdb::show_intel_syntax;		// = 1: use intel syntax for disasm
char Jdb::show_lines = 2;		// = 2: show .c and .h files lines info
					// = 1: show .c files lines info 
					// = 0: show no lines information
char Jdb::show_phys_mem;		// = 1: show physical memory
char Jdb::test_msr;			// = 1: trying to access an msr
char Jdb::gz_init_done;			// = 1: gzip module initialized
char Jdb::dump_gzip;			// = 1: currently dump to gzip

unsigned char Jdb::vga_crtc_idx_val;
unsigned int  Jdb::vga_crtc_idx;
Mword Jdb::show_tb_nr = (Mword)-1;	// current event nr
Mword Jdb::show_tb_refy;		// event nr of reference event
Mword Jdb::show_tb_absy;		// event nr of event on top of screen
char Jdb::prompt_esc[32] = "\033[32;1m"; // light green

Pic::Status Jdb::pic_status;

#ifndef CONFIG_KDB
// comes from gdb stub if compiled in
unsigned gdb_trap_recover;
#endif

static Console_buffer *console_buffer()
{
  static Console_buffer cb;
  return &cb;
}

char *Jdb::bp_mode_names[4] =
{
  "instruction", "write access", "i/o access", "read/write access"
};					// breakpoint modes

#define CONFIG_FANCY_TRAP_NAMES
#ifdef CONFIG_FANCY_TRAP_NAMES
struct Jdb::traps_t const Jdb::traps[20] = 
{ 
  /* 0*/{"DE DivError", 0},
  /* 1*/{"DB Debug", 0},
  /* 2*/{"NI NMI", 0},
  /* 3*/{"BP Breakpoint", 0},
  /* 4*/{"OF Overflow", 0},
  /* 5*/{"BR Bound range exception", 0},
  /* 6*/{"UD Invalid opcode", 0},
  /* 7*/{"NM Device not available", 0},
  /* 8*/{"DF Double fault", 0},
  /* 9*/{"OV Coprocessor segment overrun", 0},
  /*10*/{"TS Invalid TSS", 1},
  /*11*/{"NP Segment not present", 1},
  /*12*/{"SS Stack segment fault", 1},
  /*13*/{"GP General Protection fault", 1},
  /*14*/{"PF Page fault", 1},
  /*15*/{"Reserved", 0},
  /*16*/{"MF Math Fault", 0},
  /*17*/{"AC Alignment Check", 0},
  /*18*/{"MC Machine Check", 0},
  /*19*/{"XF SIMD floating point exception", 0}
};
#else
struct Jdb::traps_t const Jdb::traps[20] = 
{ 
  /* 0*/{"DE", 0},
  /* 1*/{"DB", 0}, 
  /* 2*/{"NI", 0}, 
  /* 3*/{"BP", 0}, 
  /* 4*/{"OF", 0}, 
  /* 5*/{"BR", 0},
  /* 6*/{"UD", 0}, 
  /* 7*/{"NM", 0}, 
  /* 8*/{"DF", 0}, 
  /* 9*/{"OV", 0}, 
  /*10*/{"TS", 1},
  /*11*/{"NP", 1}, 
  /*12*/{"SS", 1}, 
  /*13*/{"GP", 1}, 
  /*14*/{"PF", 1}, 
  /*15*/{"RE", 0},
  /*16*/{"MF", 0},
  /*17*/{"AC", 0},
  /*18*/{"MC", 0},
  /*19*/{"XF", 0}
};
#endif

const char * const Jdb::reg_names[] =
{ "EAX", "EBX", "ECX", "EDX", "EBP", "ESI", "EDI", "EIP", "ESP", "EFL" };

#define WEAK __attribute__((weak))

// The following function is only available if the disasm module is
// linked into the kernel. If not, disasm_bytes is 0. */
// 
// disassemble from virtual address into buffer
extern "C" unsigned
disasm_bytes(char *buffer, unsigned len, unsigned va, unsigned pa1,
	     unsigned pa2, unsigned pa_size, unsigned task,
	     int show_symbols, int show_intel_syntax) WEAK;


// The following functions are only available if the gzip module
// is linked into the kernel. If not, all symbols gz_* are 0.
// Compare ../gzip/gzip.h for definitions of function prototypes. */
// 
// initialize gzip module
extern "C" void gz_init(void *ptr, unsigned size) WEAK;
// abort gzip
extern "C" void gz_end(void) WEAK;
// open new gzip file
extern "C" int  gz_open(const char *fname) WEAK;
// write into gzip file
extern "C" int  gz_write(const char *data, unsigned int size) WEAK;
// close gzip file
extern "C" int  gz_close(void) WEAK;



STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void Jdb::init()
{
  if (strstr(Boot_info::cmdline(), " -nojdb"))
    return;

  size_t obl = 4096;
  char const *s;

  if ((s = strstr(Boot_info::cmdline(), " -out_buf=")))
    obl = strtoul(s + 10, 0, 0);

  if ((s = strstr(Boot_info::cmdline(), " -jdb_color=")))
    set_prompt_color(s[12]);

  console_buffer()->alloc(obl);
  console_buffer()->enable();

  Kconsole::console()->register_console(console_buffer());

  nested_trap_handler = base_trap_handler;
  base_trap_handler = enter_kdebugger;

  // if esc_hack, serial_esc or watchdog enabled, set slow timer handler
  set_timer_vector_run();
  static virq_t tbuf_irq(Config::TBUF_IRQ);

  Jdb_bp::init();
  Jdb_perf_cnt::init(); // needed by tbuf
  Jdb_tbuf_init::init(&tbuf_irq);

  if (gz_init!=0 && gz_open!=0 && gz_write!=0 && gz_close!=0)
    {
      char *gz_heap = 
	(char*)Kmem_alloc::allocator()->unaligned_alloc(gz_heap_pages);
      if (!gz_heap)
	panic("No memory for gz heap");

      gz_init(gz_heap, gz_heap_pages*Config::PAGE_SIZE);
      gz_init_done = 1;
    }

  // disable lbr feature per default since it eats cycles on AMD Athlon boxes
  Cpu::disable_lbr();

  // reset cursor
  blink_cursor(LAST_LINE, 0);

  _connected = true;
}

PUBLIC static inline
bool
Jdb::connected()
{
  return _connected;
}

PUBLIC static inline
int 
Jdb::source_level_debugging() 
{
  return use_nested; 
}

static 
void 
Jdb::cursor(int y, int x)
{
  printf("\033[%d;%dH", y+1, x+1);
}

static
void
Jdb::blink_cursor(int y, int x)
{
  printf("\033[%d;%df", y+1, x+1);
}

static
int
Jdb::putchar_verb(int c)
{
  if (VERBOSE)
    putchar(c);

  return c;
}

static
void
Jdb::getchar_chance()
{
  int c = Kconsole::console()->getchar(false);

  if (next_inputchar == -1)
    next_inputchar = c;
}

static
int
Jdb::getchar(void)
{
  if (VERBOSE)
    {
      if (next_inputchar != -1)
	{
	  int c = next_inputchar;
	  next_inputchar = -1;
	  return c;
	}
      return ::getchar();
    }
  else
    {
      if (debug_ctrl_len)
	{
	  debug_ctrl_len--;
	  return *debug_ctrl_str++;
	}
      return KEY_RETURN;
    }
}

static
void
Jdb::backspace_verb()
{
  if (VERBOSE)
    putstr("\b \b");
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

// Command aborted. If we are interpreting a debug command like
// enter_kdebugger("*#...") this is an error
static
void
Jdb::abort_command()
{
  if (VERBOSE)
    {
      cursor(LAST_LINE, LOGO);
      clear_to_eol();
    }

  was_error = true;
}

// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
static
void
Jdb::printf_statline(const char *format, ...)
{
  cursor(LAST_LINE, 0);
  putstr(prompt_esc);

  if (!format)
    {
      if (!current_tid.is_invalid())
	printf("(%x.%02x) ", current_tid.task(), current_tid.lthread());
    }
  putstr("jdb: \033[m");

  if (format)
    {
      va_list list;
      va_start(list, format);
      vprintf(format, list);
      va_end(list);
    }
  clear_to_eol();
}

static
void
Jdb::print_thread_state(Thread *t, unsigned cut_on_len=0)
{
  static char * const state_names[] = 
    { 
      "ready", "wait", "receiving", "polling", 
      "ipc_in_progress", "send_in_progress", "busy", "",
      "cancel", "dead", "polling_long", "busy_long",
      "", "", "rcvlong_in_progress", "",
      "fpu_owner"
    };
  unsigned i=0, comma=0, chars=0;
  do
    {
      if (t->state() & (1<<i))
	{
	  if (cut_on_len)
	    {
	      unsigned add = strlen(state_names[i]) + comma;
	      if (chars+add > cut_on_len)
		{
		  if (chars < cut_on_len-4)
		    {
		      if (dump_gzip)
			gz_write(",...", 4);
		      else
			putstr(",...");
		    }
		  break;
		}
	      chars+=add;
	    }
	  if (dump_gzip)
	    {
	      if (comma)
		gz_write(",",1);
	      gz_write(state_names[i], strlen(state_names[i]));
	    }
	  else
	    printf("%s%s", comma ? "," : "", state_names[i]);
	  comma=1;
	}
      i++;
    } while (i < (sizeof(state_names)/sizeof(char*)));
}


static int jdb_irqs_disabled;
static Proc::Status jdb_saved_flags;


// disable interrupts before entering the kernel debugger
static void save_disable_irqs()
{
  if (!jdb_irqs_disabled++)
    {
      // save interrupt flags
      jdb_saved_flags = Proc::cli_save();

#ifdef CONFIG_APIC_MASK
      apic_timer_irq_enabled = Apic::timer_is_irq_enabled();
      Apic::timer_disable_irq();
#endif
      
      Watchdog::disable();

      Jdb::pic_status = Pic::disable_all_save();
      
      if (Config::getchar_does_hlt)
	Timer::enable();

      if (Config::getchar_does_hlt)
        {
          // set timer interrupt does nothing than wakeup from hlt
          set_timer_vector_stop();
#if 0
          // Perhaps we should reduce the frequency of the timer interrupt
          // from 1024 to 32 to save more power. On the other side, it would
          // be better to not touch the RTC here...
	  Rtc::set_freq_slow();
#endif
        }
      else
        { 
          // we don't need interrupts because getchar does busy waiting
          Proc::sti_restore(jdb_saved_flags);
        }
    }
     
  if (Config::getchar_does_hlt)
    // explicit enable interrupts because the timer interrupt is
    // needed to wakeup from "hlt" state in getchar()
    Proc::sti();
}



// restore interrupts after leaving the kernel debugger
static void restore_irqs()
{
  if (!--jdb_irqs_disabled)
    {
      Proc::cli();
      
      jdb_irqs_disabled = false;

      Pic::restore_all(Jdb::pic_status);
            
      Watchdog::enable();

#ifdef CONFIG_APIC_MASK
      if (apic_timer_irq_enabled)
	Apic::timer_enable_irq();
#endif

      // reset timer interrupt vector
      if (Config::getchar_does_hlt)  
        {
          set_timer_vector_run();
#if 0
	  Rtc::set_freq_normal();
#endif
        }
         
      // reset interrupt flags
      Proc::sti_restore(jdb_saved_flags);
    }
}



// save pic state and mask all interupts
static 
void 
Jdb::open_debug_console(void)
{
  Cpu::tsc_stop();
  Jdb_bp::save_state();
  save_disable_irqs();
}

static 
void 
Jdb::close_debug_console(void)
{
  restore_irqs();
  Jdb_bp::restore_state();
  Cpu::tsc_continue();
}

static 
bool
Jdb::is_toplevel_cmd(char c)
{
  char cm[] = { c, 0 };
  Jdb_core::Cmd cmd = Jdb_core::has_cmd (cm);

  if(cmd.cmd || (0 != strchr(toplevel_cmds, c)))
    {
      next_cmd = c;
      return true;
    }

  return false;
}

static 
int
Jdb::get_num(int mask, char default_key)
{
  for(;;) 
    {
      int c=getchar();
      if (c == KEY_ESC)
	return -1;
      if ((c == KEY_RETURN) && (default_key != 0))
	c = default_key;
      if ((c > '0') && (c <= '9') && (mask & (1 << (c - '1')))) 
	{
	  putchar_verb(c);
	  return c - '0';
	}
    }
}

// request string
#define SPECIAL_SYMBOL		0x01
#define SPECIAL_ALL_PRINTABLE	0x02
static 
bool
Jdb::get_string(char *string, int size, unsigned special)
{
  for (int pos=strlen(string); ; )
    {
      string[pos] = 0;
      switch(int c=getchar())
	{
	case '0' ... '9':
	case 'a' ... 'z':
	case 'A' ... 'Z':
	case '_':
	case ':':
	case '(':
	case ')':
	case ' ':
	  if (pos < size-1)
	    {
	      putchar_verb(c);
	      string[pos++] = c;
	    }
	  break;
	case KEY_BACKSPACE:
	  if (pos)
	    {
	      backspace_verb();
	      pos--;
	    }
	  break;
	case '\t':
	  if (!special & SPECIAL_SYMBOL)
	    break;
	  if (pos < size-1)
	    {
	      if (Jdb_symbol::complete_symbol(string, size, specified_task))
		{
		  int len = strlen(string);
		  if (VERBOSE)
		    printf("%s", string + pos);
		  pos = len;
		}
	    }
	  break;
	case KEY_RETURN:
	  return true;
	case KEY_ESC:
	  abort_command();
	  return false;
	default:
	  if ((c > ' ') && (special & SPECIAL_ALL_PRINTABLE))
	    {
	      if (pos < size-1)
		{
		  putchar_verb(c);
		  string[pos++] = c;
		}
	    }
	}
    }
}

// request user for 32-bit hex input
// 'r' treated special as register indicator
// 's' treated special as symbol indicator
// 't' treated special as task indicator
// returns
//   true,  *value = ...        valid input
//   false, *value = 0          no value specified (return/enter without value)
//   false, *value = SPECIAL_T  't' (for thread/task) pressed
//   false, *value = SEPCIAL_R  'r' (for register) pressed
//   false, *value = -1         aborted/symbol not found (escape) */
#define SPECIAL_R		0x01
#define SPECIAL_T		0x02
static
bool
Jdb::x_get_32(unsigned *value, unsigned special, int first_char=0)
{
  unsigned val=0;
  
  for(int digit=0; digit<8; ) 
    {
      int c, v;

      if (first_char)
	{
	  c=v=first_char;
	  first_char=0;
	}
      else
	c=v=getchar();
      switch(c) 
	{
	case 'a' ... 'f':
	  v -= 'a' - '9' - 1;
	case '0' ... '9':
	  val = (val << 4) + (v - '0');
	  digit++;
	  putchar_verb(c);
	  break;
	case KEY_BACKSPACE:
	  if (digit) 
	    {
	      backspace_verb();
	      digit--;
	      val >>= 4;
	    }
	  break;
	case 't':
	  if (special & SPECIAL_T)
	    {
	      for (; digit--; )
		backspace_verb();
	      *value = SPECIAL_T;
	      return false;
	    }
	case 'r':
	  if (special & SPECIAL_R)
	    {
	      for (; digit--; )
		backspace_verb();
	      *value = SPECIAL_R;
	      return false;
	    }
	  break;
	case 's':
	  // search for symbol
	  if (VERBOSE)
	    putstr(" symbol:");
	  
	  char symbol[40];
	  symbol[0] = '\0';
	  if (!get_string(symbol, sizeof(symbol), SPECIAL_SYMBOL))
	    {
	      *value = 0xffffffff;
	      return false;
	    }
	  
	  if (!(val = Jdb_symbol::match_symbol_to_address
				  (symbol, strlen(symbol)>=sizeof(symbol)-1,
				  specified_task)))
	    {
	      if (VERBOSE)
		puts(" not found");
	      *value = 0xffffffff; 
	      return false; 
	    }
	  *value = val;
	  return true;
	case 'x':
	  // 
	  // If last digit was 0, delete it. This makes it possible to 
	  // cut 'n paste hex values like 0x12345678 into the serial terminal
	  if (digit && ((val & 0x10) == 0))
	    {
	      backspace_verb();
	      digit--;
	      val >>= 4;
	    }
	  break;
	case ' ':
	case KEY_RETURN: 
	  if (digit) 
	    { 
	      *value = val; 
	      return true; 
	    }
	  *value = 0; 
	  return false; 
	case KEY_ESC:
	  *value = 0xffffffff;
	  abort_command();
	  return false;
	}
    }
  *value = val;
  return true;
}

// request user for interval of [x1,x2]
static
bool
Jdb::x_get_32_interval(unsigned *low, unsigned *high)
{
  if (VERBOSE)
    putstr(" int[");
  if (!x_get_32(low, 0))
    return false;

  putchar_verb(',');
  if (!x_get_32(high, 0))
    return false;
  
  putchar_verb(']');
  return true;
}

// Get hexadecimal value with length = size in bits
// max digits = lenght / 4 */
#define DEC_VALUE		0
#define HEX_VALUE		1
static 
unsigned
Jdb::get_value(int lenght, int hex=1, int first_char=0)
{
  unsigned val=0;
 
  for (int digit=0; digit < lenght/4; )
    {
      int c, v;
      if (first_char)
	{
	  c=v=first_char;
	  first_char=0;
	}
      else
	c=v=getchar();
      switch(c)
	{
	case 'a' ... 'f':
	  if (!hex)
	    break;
	  v -= 'a' - '9' - 1;
	case '0' ... '9':
	  val = (hex ? (val << 4) : (val * 10)) + (v - '0');
	  digit++;
	  putchar_verb(c);
	  break;
	case KEY_BACKSPACE:
	  if (digit)
	    {
	      backspace_verb();
	      digit--;
	      val = (hex ? val >> 4 : val / 10);
	    }
	  break;
	case KEY_RETURN: 
	  return val;
	case ' ':
	  if (digit)
	    return val;
	  // fall through
	case KEY_ESC:
	  abort_command();
	  return 0xffffffff;
	}
    }
  return val;
}

static
int
Jdb::get_register(char *reg)
{
  char reg_name[4];
  int i;

  putchar_verb(reg_name[0] = 'E');

  for (i=1; i<3; i++)
    {
      int c = getchar();
      if (c == KEY_ESC)
	return false;
      putchar_verb(reg_name[i] = c & 0xdf);
    }

  reg_name[3] = '\0';

  for (i=0; i<9; i++)
    if (*((unsigned*)reg_name) == *((unsigned*)reg_names[i]))
      break;

  if (i==9)
    return false;
  
  *reg = i+1;
  return true;
}

static
bool
Jdb::get_task_address(unsigned *address, unsigned *task, 
		      unsigned default_addr, trap_state *ts, int first_char=0)
{
  if (!x_get_32(address, SPECIAL_R|SPECIAL_T, first_char))
    {
      if (*address == SPECIAL_T)
	{
	  putstr(" task:");
	  if ((*task = get_value(32, HEX_VALUE)) == 0xffffffff)
	    return false;

	  if (*task >= 0x00010000)
	    *task = ((L4_uid*)task)->task();

	  if (task_lookup(*task) == PAGE_INVALID)
	    {
	      puts(" Invalid task"); 
	      return false; 
	    }

	  specified_task = *task;

	  putstr(" address:");

	  return (x_get_32(address, 0) || (*address == 0));
	}
      else if (*address == SPECIAL_R)
	{
	  char reg;

	  putstr(" register:");

	  if (!get_register(&reg))
	    return false;

	  switch (reg)
	    {
	    case 1: *address = ts->eax; break;
	    case 2: *address = ts->ebx; break;
	    case 3: *address = ts->ecx; break;
	    case 4: *address = ts->edx; break;
	    case 5: *address = ts->ebp; break;
	    case 6: *address = ts->esi; break;
	    case 7: *address = ts->edi; break;
	    case 8: *address = ts->eip; break;
	    case 9: *address = ts->esp; break;
	    }

	  *task = specified_task;
	  return true;
	}
      
      if (*address != 0)
	return false;

      *address = default_addr;
    }

  *task = specified_task;
  return true;
}

static 
bool
Jdb::get_thread_id(L4_uid *tid, int first_char=0)
{
  unsigned num=0;
  int state=0, digit[2]={0,0}, long_id=0;
  for(;;)
    {
      int c, v;

      if (state == 0)
	long_id = (digit[0] > 3);
      
      if (first_char)
	{
	  c=v=first_char;
	  first_char=0;
	}
      else
	c=v=getchar();
      switch (c)
	{
	case KEY_ESC:
	  abort_command();
	  return false;
	case KEY_BACKSPACE:
	  if (digit[state])
	    {
	      num >>= 4;
	      digit[state]--;
	    }
	  else
	    if (state == 1)
	      {
		state = 0;
		num = tid->task();
	      }
	    else
	      continue;
	  backspace_verb();
	  break;
	case 'a' ... 'f':
	  v -= 'a' - '9' - 1;
	case '0' ... '9':
    	  num = (num << 4) + (v - '0');
	  digit[state]++;
	  putchar_verb(c);
	  break;
	case ' ':
    	case KEY_RETURN:
	  if (state==1)
	    {
	      tid->lthread(num);
	      return true;
	    }
	  if (digit[0])
	    {
	      if (long_id)
		{
		  *tid = num;
		}
	      else
		{
		  tid->task(num);
		  tid->lthread(0);
		}
	      return true;
	    }
	  *tid = L4_uid::INVALID;
	  return true;
	} // switch
      
      if (state==0)
	{
	  if ((digit[0] < 4) && (digit[0] > 0) && (c == '.'))
	    {
	      tid->task(num);
	      num=digit[1] = 0;
	      state = 1;
	      putchar_verb('.');
	    }
	  else if (digit[0] == 8)
	    {
	      *tid = num;
	      return true;
	    }
	}
      else // (state == 1)
	{
	  if (digit[1] == 2)
	    {
    	      tid->lthread(num);
	      return true;
	    }
	}
    } // for (;;)
}

// we do not check here if the thread id is valid because sometimes
// we want to restrict breakpoints to not exisiting threads
static
GThread_num
Jdb::get_thread_int(void)
{
  L4_uid tid;
  
  if ((!get_thread_id(&tid)) || tid.is_invalid())
    return (GThread_num)-1;
  
  return tid.gthread();
}

static
Task_num
Jdb::get_task_int(void)
{
  unsigned task;

  if (((task = get_value(32, HEX_VALUE)) == 0xffffffff) || (task == 0))
    return (Task_num)-1;

  return task;
}

// check if tid is valid and exists
static
bool
Jdb::check_thread(L4_uid tid)
{
  // L4_INVALID_ID is a valid
  if (tid.is_invalid())
    return true;
  
  return check_thread(threadid_t(&tid).lookup());
}

// check if t is valid and exists
static
bool
Jdb::check_thread(Thread *t)
{
  unsigned tcb = (unsigned)t;
  
  return (  ((tcb & ~(Config::thread_block_size * ( 1<< (7+11)) - 1)) 
	    == Kmem::mem_tcbs)
	  && (Kmem::virt_to_phys(reinterpret_cast<void*>(tcb))!=PAGE_INVALID));
}

static 
Thread*
Jdb::get_thread(trap_state *ts)
{
  unsigned long esp = (unsigned long) ts;
  if (   esp < Kmem::mem_tcbs
      || esp > Kmem::mem_tcbs + ~Config::thread_block_mask)
    return reinterpret_cast<Thread*>(Kmem::mem_tcbs);
  
  return reinterpret_cast<Thread *>(esp & ~(Config::thread_block_size - 1));
}

// determine address of page table for a specific task
// return PAGE_INVALID if address space does not exist
static
vm_offset_t
Jdb::task_lookup(unsigned task)
{
  if (task >= 2048)
    return PAGE_INVALID;
  
  Space *s = (task == 0)
		  ? (Space *)Kmem::dir()
		  : Space_index(task).lookup();

  if (!s)
    return PAGE_INVALID;
  
  vm_offset_t top_a = (vm_offset_t)s->virt_to_phys((vm_offset_t) s);

  // XXX sigma0 hack
  if (task == 2)
    top_a &= HI_MASK;

  return top_a;
}

static
unsigned
establish_phys_mapping(Address address, Address *offset)
{
  unsigned pte = pa_to_pte(address);

  // setup adapter page table entry
  *Kmem::jdb_adapter_pt = INTEL_PTE_VALID | INTEL_PTE_WRITE | INTEL_PTE_WTHRU |
   			  INTEL_PTE_NCACHE | INTEL_PTE_REF | INTEL_PTE_MOD |
			  Kmem::pde_global() | pte;
  Kmem::tlb_flush(Kmem::jdb_adapter_page);

  *offset = address - pte;

  return Kmem::jdb_adapter_page;
}

extern "C"
const char*
disasm_get_symbol_at_address(unsigned address, unsigned task)
{
  return Jdb_symbol::match_address_to_symbol(address, task);
}

static
bool
Jdb::disasm_line(char *buffer, int buflen,
		 unsigned *va, int show_symbols, unsigned task)
{
  int len;
  unsigned pa1, pa2=0, size;

  if (task>=2048)
    {
      printf("Invalid task %x\n", task);
      *va += 1;
      return false;
    }

  if ((pa1 = k_lookup(*va, task)) != PAGE_INVALID)
    {
      // deal with page boundaries
      size = (Config::PAGE_SIZE - (pa1 & ~Config::PAGE_MASK));
      if ((size < 64) && ((pa2 = k_lookup(*va+size, task)) != PAGE_INVALID))
	size = 64;
      if ((len = disasm_bytes(buffer, buflen, *va, pa1, pa2, size,
			      task, show_symbols, show_intel_syntax)) < 0)
	return false;
      
      *va += len;
      return true;
    }

  *va += 1;
  return false;
}

static
unsigned
Jdb::disasm_offset(unsigned *start, int offset, unsigned task)
{
  if (offset>0)
    {
      unsigned va = *start;
      while (offset--)
	{
	  if (!disasm_line(0, 0, &va, 0, task))
	    {
	      *start = va + offset;
	      return false;
	    }
	}
      *start = va;
      return true;
    }
  else
    {
      while (offset++)
	{
	  unsigned va=*start-64, va_start;
	  for (;;)
	    {
	      va_start = va;
	      if (!disasm_line(0, 0, &va, 0, task))
		{
		  *start += (offset-1);
		  return false;
		}
	      if (va >= *start)
		break;
	    }
	  *start = va_start;
	}

      return true;
    }
}

static
bool
Jdb::show_disasm_line(int len, unsigned *va,
		      int show_symbols, unsigned task)
{
  int clreol = 0;
  if (len < 0)
    {
      len = -len;
      clreol = 1;
    }
  
  if (disasm_bytes != 0)
    {
      char *line = (char*)alloca(len);
      if (line && disasm_line(line, len, va, show_symbols, task))
	{
	  if (clreol)
	    printf("%s\033[K\n", line);
	  else
	    printf("%-*s\n", len, line);
	  return true;
	}
    }
  
  if (clreol)
    puts("........\033[K");
  else
    printf("........%*s", len-8, "\n");
  return false;
}

static
bool
Jdb::disasm_address_space(unsigned address, unsigned task, int level=0)
{
  if (disasm_bytes == 0)
    return false;

  if (level==0)
    fancy_clear_screen();

  for (;;)
    {
      cursor(0, 0);
      
      for (unsigned i=24, a=address; i>0; i--)
	{
	  const char *symbol;
      	  char str[78], *nl;
	  char stat_str[6];

	  *(unsigned int*  ) stat_str    = 0x20202020;
	  *(unsigned short*)(stat_str+4) = 0x0020;
	  
	  if ((symbol = Jdb_symbol::match_address_to_symbol(a, task)))
	    {
	      str[0] = '<';
    	      strncpy(str+1, symbol, sizeof(str)-3);
	      str[sizeof(str)-3] = '\0';
	      
	      // cut symbol at newline
	      for (nl=str; (*nl!='\0') && (*nl!='\n'); nl++)
		;
	      *nl++ = '>';
	      *nl++ = ':';
    	      *nl++ = '\0';
	      
	      printf("\033[33;1m%s\033[m\033[K\n", str);
	      if (!--i)
		break;
	    }
	 
	  if (show_lines)
	    {
	      if (Jdb_lines::match_address_to_line(a, task, str, sizeof(str)-1,
						   show_lines==2))
		{
		  printf("\033[37m%s\033[m\033[K\n", str);
		  if (!--i)
		    break;
		}
	    }
	 
	  // show breakpoint number
	  if (Mword i=Jdb_bp::check_bp_set(a))
	    {
	      stat_str[0] = '#';
    	      stat_str[1] = '0'+i;
	    }

	  printf("%08x %s", a, stat_str);
	  show_disasm_line(-64, &a, 1, task);
	}
      
      printf_statline("u<%08x> task %-3x  %s  %s%35s",
		      address, task, 
		      show_lines ? show_lines > 1 ?
			  "[Headers]" : "[Source] " : "         ",
		      show_intel_syntax ? "[Intel]"   : "[AT&T] ",
		      "KEYS: Arrows PgUp PgDn");
     
      cursor(LAST_LINE, LOGO+1);
      switch (int c=getchar())
	{
	case KEY_CURSOR_LEFT:
	  address -= 1;
	  break;
	case KEY_CURSOR_RIGHT:
	  address += 1;
	  break;
	case KEY_CURSOR_DOWN:
	  disasm_offset(&address, +1, task);
	  break;
	case KEY_CURSOR_UP:
	  disasm_offset(&address, -1, task);
	  break;
	case KEY_PAGE_UP:
	  disasm_offset(&address, -23, task);
	  break;
	case KEY_PAGE_DOWN:
	  disasm_offset(&address, +23, task);
	  break;
	case ' ':
	  show_lines = (show_lines+1) % 3;
	  break;
	case KEY_TAB:
	  show_intel_syntax = !show_intel_syntax;
	  break;
	case KEY_CURSOR_HOME:
	  if (level > 0)
	    return true;
	  break;
	case KEY_ESC:
	  abort_command();
	  return false;
	default:
	  if (is_toplevel_cmd(c)) 
	    return false;
	  break;
	}
    }
  
  return true;
}


extern "C" char in_slowtrap, in_page_fault, in_handle_fputrap;
extern "C" char in_interrupt, in_timer_interrupt, in_timer_interrupt_slow;
extern "C" char ret_switch WEAK, se_ret_switch WEAK, in_slow_ipc1 WEAK;
extern "C" char in_slow_ipc2 WEAK, in_slow_ipc3 WEAK, in_slow_ipc4;
extern "C" char in_slow_ipc5, in_slow_ipc6 WEAK, in_sc_ipc1 WEAK;
extern "C" char in_sc_ipc2 WEAK;

// Try to guess the thread state of t by walking down the kernel stack and
// locking at the first return address we find.
static
Jdb::Guessed_thread_state
Jdb::guess_thread_state(Thread *t)
{
  Guessed_thread_state state = s_unknown;
  Mword *ktop = (Mword*)((((Mword)t->kernel_sp) & ~0x7ff) + 0x800);

  for (int i=-1; i>-22; i--)
    {
      if (ktop[i] != 0)
	{
	  if (ktop[i] == (Mword)&in_page_fault)
	    state = s_pagefault;
	  if ((ktop[i] == (Mword)&ret_switch) ||    // shortcut.S, int 0x30
	      (ktop[i] == (Mword)&in_slow_ipc1) ||  // shortcut.S, int 0x30
	      (ktop[i] == (Mword)&se_ret_switch) || // shortcut.S, sysenter
	      (ktop[i] == (Mword)&in_slow_ipc2) ||  // shortcut.S, sysenter
	      (ktop[i] == (Mword)&in_slow_ipc3) ||  // entry.S, int 0x30
	      (ktop[i] == (Mword)&in_slow_ipc4) ||  // entry.S, int 0x30 log
	      (ktop[i] == (Mword)&in_slow_ipc5) ||  // entry.S, sysenter log
	      (ktop[i] == (Mword)&in_slow_ipc6) ||  // entry.S, sysenter
	      (ktop[i] == (Mword)&in_sc_ipc1)   ||  // entry.S, int 0x30
	      (ktop[i] == (Mword)&in_sc_ipc2))      // entry.S, sysenter
	    state = s_ipc;
	  else if (ktop[i] == (Mword)&in_handle_fputrap)
	    state = s_fputrap;
	  else if (ktop[i] == (Mword)&in_interrupt)
	    state = s_interrupt;
	  else if ((ktop[i] == (Mword)&in_timer_interrupt) ||
		   (ktop[i] == (Mword)&in_timer_interrupt_slow))
	    state = s_timer_interrupt;
	  else if (ktop[i] == (Mword)&in_slowtrap)
	    state = s_slowtrap;
	  if (state != s_unknown)
	    break;
	}
    }

  if (state == s_unknown && (t->state() & Thread_ipc_mask))
    state = s_ipc;

  return state;
}

static
void
Jdb::info_thread_state(Thread *t, Guessed_thread_state state)
{ 
  Mword *ktop = (Mword*)((((Mword)t->kernel_sp) & ~0x7ff) + 0x800);
  int sub = 0;

  switch (state)
    {
    case s_ipc:
      if (state == s_ipc)
	{
	  printf("EAX=%08x  ESI=%08x\n"
		 "EBX=%08x  EDI=%08x\n"
		 "ECX=%08x  EBP=%08x\n"
		 "EDX=%08x  ESP=%08x  SS=%04x\n"
		 "in ipc (user level registers)",
		 ktop[ -6], ktop[-10], ktop[ -8], ktop[ -9], 
		 ktop[-12], ktop[ -7], ktop[-11], ktop[ -2], 
		 ktop[ -1] & 0xffff);
	}
      break;
    case s_fputrap:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in exception #0x07 (user level registers)",
	     ktop[-7], ktop[-8], ktop[-9], ktop[-2], ktop[-1] & 0xffff);
      break;
    case s_pagefault:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x  EBP=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in page fault, error %08x (user level registers)\n"
	     "\n"
	     "page fault linear address %08x",
	     ktop[-7], ktop[-8], ktop[-6], ktop[-9], ktop[-2],
	     ktop[-1] & 0xffff, ktop[-10], ktop[-11]);
      break;
    case s_interrupt:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in interrupt #0x%02x (user level registers)",
	     ktop[-6], ktop[-8], ktop[-7], ktop[-2], ktop[-1] & 0xffff,
	     ktop[-9]);
      break;
    case s_timer_interrupt:
#ifndef CONFIG_NO_FRAME_PTR
      sub = -1;
#endif
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in timer interrupt (user level registers)",
	     ktop[-6-sub], ktop[-8-sub], ktop[-7-sub], ktop[-2],
	     ktop[-1] & 0xffff);
      break;
    case s_slowtrap:
      printf("EAX=%08x  ESI=%08x  DS=%04x\n"
             "EBX=%08x  EDI=%08x  ES=%04x\n"
             "ECX=%08x  EBP=%08x  GS=%04x\n"
             "EDX=%08x  ESP=%08x  SS=%04x\n"
             "in exception %d, error %08x (user level registers)",
	     ktop[ -9], ktop[-15], ktop[-17] & 0xffff, 
	     ktop[-12], ktop[-16], ktop[-18] & 0xffff,
	     ktop[-10], ktop[-14], ktop[-19] & 0xffff,
	     ktop[-11], ktop[ -2], ktop[ -1] & 0xffff,
	     ktop[ -8], ktop[ -7]);
      break;
    case s_unknown:;
    }
}

static
bool
Jdb::show_disasm(trap_state *ts)
{
  unsigned task, address;
 
  if (disasm_bytes == 0)
    return false;
  if (!get_task_address(&address, &task, ts->eip, ts))
    return false;

  return disasm_address_space(address, task);
}

static 
bool
Jdb::show_tcb(trap_state *ts, L4_uid tid, int level=0)
{
  
/*
thread: 0081 (001.01) <00020401 00080000>	                        prio: 10
state : 85, ready                            lists: 81                   mcp: ff

wait for: --                             rcv descr: 00000000   partner: 00000000
sndq    : 0081 0081                       timeouts: 00000000   waddr0/1: 000/000
cpu time: 0000000000 timeslice: 01/0a

pager   : --                            prsent lnk: 0080 0080
ipreempt: --                            ready link : 0080 0080
xpreempt: --
                                        soon wakeup lnk: 
EAX=00202dfe  ESI=00020401  DS=0008     late wakeup lnk: 
EBX=00000028  EDI=00080000  ES=0008
ECX=00000003  EBP=e0020400
EDX=00000001  ESP=e00207b4

700:
720:
740:
760:
780:
7a0:                                                  0000897b 00000020 00240082
7c0:  00000000 00000000 00000000 00000000    00000000 00000000 00000000 00000000
7e0:  00000000 00000000 ffffff80 00000000    0000001b 00003200 00000000 00000013
L4KD: 
*/

 new_tcb:

  threadid_t t_id(&tid);
  Thread *t = current_active;
  bool screen_update = true, is_current_thread = true;

  if (t_id.is_valid() && (t_id.lookup() != t))
    {
      is_current_thread = false;
      t = t_id.lookup();
    }

  if (!check_thread(t) || !t->state())
    {
      puts(" Invalid thread");
      return false;
    }

  unsigned ksp = (unsigned)t->kernel_sp;
  unsigned start_addr = ksp & 0xffffffe0;
  unsigned start_line = start_addr & 0xfff;
  unsigned *current;

  unsigned ksp_off = ((ksp & 0xfff) - start_line) / 4;
  unsigned acty = 16, actx = ksp_off;

  unsigned absy = ((ksp & (Config::thread_block_size - 1)) / 32);
  unsigned max_absy = Config::thread_block_size / 32 - 8;

  if (level==0)
    {
      fancy_clear_screen();
      screen_update = false;
    }
  
 whole_screen:
  
  if (screen_update)
    {
      home();
      for(int i=0; i<24; i++)
	{
	  clear_to_eol();
	  putchar('\n');
	}
      home();
      screen_update = false;
    }
  
  char tid_buf[32];
#ifdef CONFIG_ABI_X0
  sprintf(tid_buf, "<%08x>         ",
	  (Unsigned32)(t->_id.raw()));
#else
  sprintf(tid_buf, "<%08x %08x>",
	  (Unsigned32)(t->_id.raw() >> 32), (Unsigned32)(t->_id.raw()));
#endif
  printf("thread: %3x.%02x %s\t\t\tprio: %02x\tmcp: %02x\n",
	 t->_id.task(), t->_id.lthread(), tid_buf,
	 t->sched()->prio(), t->sched()->mcp());

  printf("state: %03x ", t->state());
  print_thread_state(t);

  putstr("\n\nwait for: ");

  if (t->partner()
      && (t->state() & (Thread_receiving | Thread_busy | 
			Thread_rcvlong_in_progress)))
    {
      if (   t->partner()->_id.is_irq())
	printf("irq %02x", t->partner()->_id.irq());
      else
	printf("%3x.%02x",
	       t->partner()->_id.task(), t->partner()->_id.lthread());
    }
  else
    putstr("---.--");

  putstr("   polling: ");

  if (t->_send_partner)
    printf("%3x.%02x",
	   Thread::lookup(context_of(t->_send_partner))->_id.task(), 
	   Thread::lookup(context_of(t->_send_partner))->_id.lthread());
  else
    putstr("---.--");

  putstr("\trcv descr: ");
  
  if (t->state() & Thread_ipc_receiving_mask)
    printf("%08x", t->receive_regs()->rcv_desc().raw());
  else
    putstr("        ");

  putstr("   partner: ---.--\n"
         "reqst.to: ");
  
  if (t->thread_lock()->lock_owner())
    printf("%3x.%02x", 
	  Thread::lookup(t->thread_lock()->lock_owner())->_id.task(), 
	  Thread::lookup(t->thread_lock()->lock_owner())->_id.lthread());
  else
    putstr("---.--");

 putstr("\t\t\ttimeout: ");

  if (t->_timeout && t->_timeout->is_set())
    {
      Signed64 diff = (t->_timeout->get_timeout()) * 1000;
      char buffer[20];
      int len;
      if (diff < 0)
	diff = 0;
      len = write_ns(diff, buffer, sizeof(buffer)-1, false, false);
      buffer[len] = '\0';
      printf("%s", buffer);
    }
  
  Unsigned64 cputime = t->sched()->get_total_cputime();
  printf("\ncpu time: %02x%08x timeslice: %02x/%02x\n"
         "pager\t: ",
	  (unsigned)(cputime >> 32), (unsigned)cputime,
	  t->sched()->ticks_left(), t->sched()->timeslice());
  if (t->_pager)
    printf("%3x.%02x",
	   t->_pager->_id.task(), t->_pager->_id.lthread());
  else
    putstr("---.--");

  putstr("\nipreempt: ---.--\t\t\tready lnk : ");

  if (t->state() & Thread_running) 
    {
      if (t->ready_next)
	printf("%3x.%02x ",
		Thread::lookup(t->ready_next)->_id.task(), 
		Thread::lookup(t->ready_next)->_id.lthread());
      else
      	putstr("???.?? ");
      if (t->ready_prev)
	printf("%3x.%02x\n",
		Thread::lookup(t->ready_prev)->_id.task(), 
		Thread::lookup(t->ready_prev)->_id.lthread());
      else
      	puts("???.??");
    }
  else
    puts("---.-- ---.--");

  putstr("xpreempt: ---.--\t\t\tprsent lnk: ");
  if (t->present_next)
    printf("%3x.%02x",
	   t->present_next->_id.task(), t->present_next->_id.lthread());
  else
    putstr("---.--");
  if (t->present_prev)
    printf(" %3x.%02x\n",
	 t->present_prev->_id.task(), t->present_prev->_id.lthread());
  else
    puts(" ---.--");

  if (is_current_thread)
    {
      int from_user = (ts->cs & 3);
      
      // registers, disassemble
      printf("EAX=%08x  ESI=%08x  DS=%04x\n"
             "EBX=%08x  EDI=%08x  ES=%04x\n"
             "ECX=%08x  EBP=%08x  GS=%04x\n"
             "EDX=%08x  ESP=%08x  SS=%04x\n"
             "trapno %d, error %08x, from %s mode\n"
             "CS=%04x  EIP=\033[33;1m%08x\033[m  EFlags=%08x kernel ESP=%08x",
	     ts->eax, ts->esi, ts->ds & 0xffff,
	     ts->ebx, ts->edi, ts->es & 0xffff,
	     ts->ecx, ts->ebp, ts->gs & 0xffff,
	     ts->edx, from_user ? ts->esp : (unsigned)&ts->esp,
	     from_user ? ts->ss & 0xffff : get_ss(),
	     ts->trapno, ts->err, from_user ? "user" : "kernel",
	     ts->cs & 0xffff, ts->eip, ts->eflags, ksp);

      if (ts->trapno == T_PAGE_FAULT)
	printf("\npage fault linear address %08x\n", ts->cr2);
      
      // disassemble two lines at EIP
      unsigned disass_addr = ts->eip;
      putstr("\033[33;1m");
      cursor(10,40);
      show_disasm_line(-40, &disass_addr, 0, t->_id.task());
      cursor(11,40);
      putstr("\033[m");
      show_disasm_line(-40, &disass_addr, 0, t->_id.task());
      putstr("\033[m");
    }
  else if (t->_id.task() != 0)
    {
      Mword *k_top = (Mword*)((ksp & ~0x7ff) + 0x800);
      info_thread_state(t, guess_thread_state(t));
      cursor(14, 0);
      printf("CS=%04x  EIP=\033[33;1m%08x\033[m  EFlags=%08x kernel ESP=%08x",
	      k_top[-4] & 0xffff, k_top[-5], k_top[-3], ksp);
      // disassemble two lines at EIP
      unsigned disass_addr = k_top[-5];
      putstr("\033[33;1m");
      cursor(10,40);
      show_disasm_line(-40, &disass_addr, 0, t->_id.task());
      cursor(11,40);
      putstr("\033[m");
      show_disasm_line(-40, &disass_addr, 0, t->_id.task());
      putstr("\033[m");
    }
  else
    {
      // kernel thread
      cursor(14, 0);
      printf("kernel ESP=%08x", ksp);
    }

dump_stack:

  cursor(16, 0);
  start_line = start_addr & 0xfff;

  // dump the stack from ksp bottom right to tcb_top
  unsigned p = start_addr;
  for (int y=0; y<8; y++)
    {
      getchar_chance();

      printf("%03x:", p & 0xfff);
      for (int x=0; x <= 7; x++)
	{
	  if ((p & 0x800) != (ksp & 0x800))
	    putstr("         ");
	  else
	    {
	      // It is possible that we have an invalid kernel_sp
	      // (in result of an kernel error). Handle it smoothly.
	      unsigned va = k_lookup(p, t->_id.task());
  	      if (va != PAGE_INVALID)
    		printf(" %s%08x%s", 
		      ((p & 0x7ff) >= 0x7ec) ? "\033[36;1m" : "",
		      *(unsigned*)p,
		      ((p & 0x7ff) >= 0x7ec) ? "\033[m" : "");
	      else
		putstr(" ........");
	    }
	  p+=4;
	}
      putchar('\n');
    }

  printf_statline("t%73s",
                  "KEYS: <e>dit <r>dy/<p>rsnt <p>rev/<n>ext Arrows PgUp PgDn");

  for (;;)
    {
      current = (unsigned *)(start_addr + (acty-16)*0x20 + actx*4);
      if (((Mword)current & 0x800) == (ksp & 0x800))
	{
	  cursor(acty, 9*actx+5);
	  printf("\033[33;1m%08x\033[m", *current);
	}
      cursor(acty, 9*actx+5);
      int c=getchar();
      if (((Mword)current & 0x800) == (ksp & 0x800))
	{
	  cursor(acty, 9*actx+5);
	  printf("\033[%sm%08x\033[m",
		 (((Mword)current & 0x7ff) >= 0x7ec) ? "36;1" : "",
	         *current);
	}
      cursor(LAST_LINE, LOGO);
      switch (c) 
	{
	case KEY_CURSOR_LEFT:
	  if (actx > 0) 
	    actx--;
	  else if (acty > 16)
	    { 
      	      acty--; 
	      actx=7;
	    }
	  else if (absy > 0)
    	    {
	      absy--; 
	      actx=7;
	      start_addr-=0x20; 
	      break;
	    }
  	  continue;
	case KEY_CURSOR_RIGHT:
	  if (actx < 7)
	    actx++;
	  else if (acty < 23)
	    { 
      	      acty++; 
	      actx=0;
	    }
	  else if (absy < max_absy)
	    {
	      absy++;
    	      actx=0;
	      start_addr+=0x20;
	      break;
	    }
	  continue;
	case KEY_CURSOR_UP:
	  if (acty > 16) 
	    acty--; 
	  else if (absy > 0) 
	    { 
      	      absy--; 
	      start_addr-=0x20;
	      break;
	    }
	  continue;
	case KEY_CURSOR_DOWN:
	  if (acty < 23) 
	    acty++; 
	  else if (absy < max_absy) 
	    {
      	      absy++; 
	      start_addr+=0x20;
	      break;
	    }
	  continue;
	case KEY_PAGE_UP:
	  if (absy > 8)
	    { 
      	      absy-=8;
	      start_addr-=8*0x20;
	      break;
	    }
	  else if (absy > 0)
	    {
	      start_addr-=absy*0x20;
	      absy=0;
	      break;
	    }
	  else if (acty > 16)
	    acty=16;
	  continue;
	case KEY_PAGE_DOWN:
	  if (absy < max_absy-7)
	    {
      	      absy+=8;
	      start_addr+=8*0x20;
	      break;
	    }
	  else if (absy < max_absy)
	    {
	      start_addr += (max_absy-absy)*0x20;
	      absy=max_absy;
	      break;
	    }
	  else if (acty < 23)
	    acty = 23;
       	  continue;
	case KEY_RETURN:
	  if (((unsigned)current & 0x800) == (ksp & 0x800))
	    {
	      if (!dump_address_space(ts, D_MODE, (Address)*current,
				      t->_id.task(), level+1))
		return false;
	      screen_update = true;
	      break;
	    }
	  continue;
	case ' ':
	  // if not linked, do nothing
	  if ((disasm_bytes != 0)
	      && (((unsigned)current & 0x800) == (ksp & 0x800)))
	    {
    	      if (!disasm_address_space(*current, t->_id.task(), level+1))
		return false;
	      screen_update = true;
	      break;
	    }
	  continue;
	case 'u':
	  if ((disasm_bytes != 0)
	      && (((unsigned)current & 0x800) == (ksp & 0x800)))
	    {
	      printf_statline("u[address=%08x task=%x] ", 
		  (Address)*current, t->_id.task());
	      int c1 = getchar();
    	      if (c1 != KEY_RETURN)
		{
	    	  unsigned new_addr, new_task;

    		  printf_statline(0);
		  putchar('u');
		  if (!get_task_address(&new_addr, &new_task, ts->eip, ts, c1))
		    return false;

		  disasm_address_space(new_addr, new_task);
		  return false;
		}

	      if (!disasm_address_space((Address)*current, t->_id.task(), 
					level+1))
    		return false;

	      screen_update = true;
	      break;
	    }
	  continue;
	case 'r': // ready-list
	  putchar(c);
     	  if (!(tid = get_ready()).is_invalid())
	    goto new_tcb;
	  break;
	case 'p': // present-list or show_pages
	  putchar(c);
	  switch (c=getchar()) 
	    {
	    case 'n':
	    case 'p':
	    case KEY_ESC:
	      if (!(tid = get_present(c)).is_invalid())
		goto new_tcb;
	      break;
	    default:
	      if (!show_pages(ts, c))
		return false;
	      break;
	    }
	  break;
	case 'e':
	  if (((unsigned)current & 0x800) == (ksp & 0x800))
	    {
	      Mword value;
	      int c;

	      cursor(acty, 9*actx+5);
	      printf("        ");
	      printf_statline("edit <%08x> = %08x%s",
			      current, *current, is_current_thread
			        ? "  (press <Space> to edit registers)" : "");
	      cursor(acty, 9*actx+5);
	      c = getchar();
	      if (c==KEY_ESC)
		break;
	      if (c != ' ' || !is_current_thread)
		{
		  // edit memory
		  putchar(c);
		  printf_statline("edit <%08x> = %08x", current, *current);
		  cursor(acty, 9*actx+5);
		  if (!x_get_32(&value, 0, c))
		    {
		      cursor(acty, 9*actx+5);
		      printf("%08x", *current);
		      break;
		    }
		  else
		  *current = value;
		}
	      else
		{
		  // edit registers
		  char reg;
		  unsigned *reg_ptr=0, x=0, y=0;

		  cursor(acty, 9*actx+5);
	      	  printf("%08x", *current);

		  printf_statline("edit register "
				  "e{ax|bx|cx|dx|si|di|sp|bp|ip|fl}: ");
		  cursor(LAST_LINE, 53);
		  if (!get_register(&reg))
		    break;

		  switch (reg)
		    {
		    case  1: x =  4; y =  9; reg_ptr = &ts->eax; break;
		    case  2: x =  4; y = 10; reg_ptr = &ts->ebx; break;
		    case  3: x =  4; y = 11; reg_ptr = &ts->ecx; break;
		    case  4: x =  4; y = 12; reg_ptr = &ts->edx; break;
		    case  5: x = 18; y = 11; reg_ptr = &ts->ebp; break;
		    case  6: x = 18; y =  9; reg_ptr = &ts->esi; break;
		    case  7: x = 18; y = 10; reg_ptr = &ts->edi; break;
		    case  8: x = 13; y = 14; reg_ptr = &ts->eip; break;
		    case  9: x = 18; y = 12; reg_ptr = &ts->esp; break;
		    case 10: x = 35; y = 12; reg_ptr = &ts->eflags; break;
		    }

		  cursor(y, x);
		  putstr("        ");
		  printf_statline("edit %s = %08x", reg_names[reg-1], *reg_ptr);
		  cursor(y, x);
		  if (x_get_32(&value, 0))
		    *reg_ptr = value;
		  screen_update = true;
		  break;
		}
	    }
	  break;
	case KEY_CURSOR_HOME:
	  if (level > 0)
	    return true;
	  continue;
	case KEY_ESC:
	  abort_command();
	  return false;
	default:
	  if (is_toplevel_cmd(c)) 
	    return false;
	  continue;
	}
      if (screen_update)
	goto whole_screen;
      else
	goto dump_stack;
    }
} 

static 
L4_uid
Jdb::get_ready()
{
  Thread *t = current_ready ? current_ready : current_active;
  switch(getchar()) 
    {
    case 'n':
      if (t->ready_next)
	return (current_ready = Thread::lookup(t->ready_next))->_id;
      break;
    case 'p':
      if (t->ready_prev)
	return (current_ready = Thread::lookup(t->ready_prev))->_id;
      break;
    default:
      abort_command();
      break;
    }
  return L4_uid::INVALID;
}

static 
L4_uid
Jdb::get_present(int first_char=0)
{
  Thread *t = current_present ? current_present : current_active;
  int c=first_char;
  if (!c)
    c=getchar();
  switch(c) 
    {
    case 'n':
      if (t->present_next)
	return (current_present = t->present_next)->_id;
      break;
    case 'p':
      if (t->present_prev)
	return (current_present = t->present_prev)->_id;
      break;
    case KEY_ESC:
      abort_command();
      break;
    }
  return L4_uid::INVALID;
}

static 
bool
Jdb::search_cmd(trap_state *ts)
{
  unsigned string;
  unsigned address = 0;
  
  putstr("earch dword:");

  if (!x_get_32(&string, 0))
    return false;

  for (bool research=true; research; )
    {
      if (search_dword(string, &address))
	{
	  unsigned pa = Kmem::virt_to_phys((unsigned *)address);
	  
	  cursor(LAST_LINE, 0);
      	  printf("  dword %08x found at %08x\033[K\n", string, pa);
	  printf_statline("s %08x%63s", string, "KEYS: <n>ext <d>ump");
          
	  switch (int c=getchar())
	    {
	    case 'd':
	      if (!dump_address_space(ts, D_MODE, (Address)pa, 0, 1))
		return false;
	      break;
	    case 'n':
	    case KEY_RETURN:
	      address++;
	      research = true;
	      break;
	    default:
	      if (is_toplevel_cmd(c))
		return false;
	      research = false;
	      break;
	    }
	}
      else
	{
	  cursor(LAST_LINE, 0);
      	  printf("  dword %08x not found\033[K\n", string);
	  research = false;
	}
    }
  return true;
}    

static
void
Jdb::set_single_step(trap_state *ts)
{
  switch(getchar()) 
    {
    case '+':
      ts->eflags |= EFLAGS_TF;
      single_step_cmd = true;
      if (VERBOSE)
	puts("+");
      break;
    case '-':
      ts->eflags &= ~EFLAGS_TF;
      single_step_cmd = false;
      if (VERBOSE)
	puts("-");
      break;
    default:
      abort_command();
      break;
    }
}

static
void
Jdb::show_tcb_cmd(trap_state *ts)
{
  L4_uid tid;
  
  switch (int c = getchar())
    {
    case '+':
    case '-':
      auto_tcb = (c == '+');
      putchar_verb(c);
      putchar_verb('\n');
      break;
    default:
      if (get_thread_id(&tid, c))
	{
	  threadid_t t_id(&tid);
	  Thread *t = current_active;
	  
	  current_ready = t;
      	  current_present = ((t_id.is_valid() && (t != t_id.lookup())))
			     ? t_id.lookup() : 0;
	  show_tcb(ts, tid);
	}
      abort_command();
      break;
    }
}

static 
void
Jdb::show_io_cmd(void)
{
  unsigned task_number;
  Space * task = 0;
  
  if (!x_get_32(&task_number, 0, 0)) 
    {
      if (task_number != 0)
	return;
      
      if (!current_tid.is_invalid())
	task = 	Space_index(current_tid.task()).lookup();
    }
  else if (task_number < 2048)  // interpret as task number
    task = Space_index(task_number).lookup();
  else
    return;

  if (!task)
    {
      puts(" Invalid task");
      return;
    }

  // base addresses of the two IO bitmap pages
  vm_offset_t bitmap_1, bitmap_2; 
  bitmap_1 = task->lookup(Kmem::io_bitmap);
  bitmap_2 = task->lookup(Kmem::io_bitmap + Config::PAGE_SIZE);
  fancy_clear_screen();

  printf("\nIO bitmap for task %x\n", (unsigned)task->space());
  if(bitmap_1 == 0xffffffff && bitmap_2 == 0xffffffff)
    { // no memory mapped for the IO bitmap
      putstr("No memory mapped");
      return;
    }
  else 
    {
      if(bitmap_1 != 0xffffffff)
	printf("First page mapped to  %08x; ", bitmap_1);
      else
	putstr("First page unmapped; ");

      if(bitmap_2 != 0xffffffff)
	printf("Second page mapped to %08x\n", bitmap_2);
      else
	puts("Second page unmapped");
    }

  puts("\nPorts assigned:");

  bool mapped= false;
  unsigned count=0;
  unsigned i;

  for( i = 0; i < L4_fpage::IO_PORT_MAX; i++ )
    {
      if(task->io_lookup(i) != mapped)
	{
	  if(! mapped)
	    {
	      mapped = true;
	      printf("%04x-", i);
	    }
	  else
	    {
	      mapped = false;
	      printf("%04x, ", i-1);
	    }
	}
      if(mapped)
	count++;
    }
  if(mapped)
    printf("%04x, ", L4_fpage::IO_PORT_MAX -1);

  printf("\nPort counter: %d ", task->get_io_counter() );
  if(count == task->get_io_counter())
    puts("(correct)");
  else
    printf(" !! should be %d !!\n", count);

  return;	     
}


static 
bool
Jdb::set_breakpoint(unsigned mode, trap_state *ts)
{
  unsigned address, dummy_task, len = 0;
  int i = Jdb_bp::first_unused();
 
  if (i<4) 
    {
      if (VERBOSE)
	putstr(" at:");

      if (!get_task_address(&address, &dummy_task, 0, ts))
	return false;

      if (mode != 0)
	{
	  // instruction breakpoints should have a length of 1
	  if (VERBOSE)
	    putstr(" len:");
	  if ((len = get_num(0xb, '4')) == (unsigned)-1) // 1011 => 4,2,1
	    return false;

	  len--;
	}

      // align breakpoint address
      address &= ~len;
      
      // set breakpoint
      Jdb_bp::set_bp(i, address + bp_base, mode, len+1);
      
      putchar_verb('\n');
      return true;
    }
  
  if (VERBOSE)
    puts(" No breakpoint available");
  return false;
}

static 
int
Jdb::get_br_num()
{
  switch (int c=getchar()) 
    {
    case '1' ... '4':
      putchar_verb(c);
      return (c - '1');
      break;
    }
  abort_command();
  return -1;
}

static 
bool 
Jdb::restrict_breakpoint()
{
  int other;
  char reg;
  int bpn, thread;
  unsigned address, len, vy, vz, ry, rz;
  
  // select breakpoint (1..4)
  if (VERBOSE)
    putstr(" bpn:");
  if ((bpn = get_br_num()) == -1)
    return false;

  putchar(' ');
  Jdb_bp::get_bpres(bpn, 
		    &other, &thread, 
		    &reg, &ry, &rz, &len, 
		    &address, &vy, &vz);

  switch (int c=getchar())
    {
    case '-':
      if (VERBOSE)
	puts("(bp restrictions deleted)");
      Jdb_bp::clear_breakpoint_restriction(bpn);
      return true;
    case 'T':
    case 't':
      if (VERBOSE)
	printf("%chread:",c);
      if ((thread = get_thread_int()) == -1)
	return false;
      other = (c == 'T');
      break;
    case 'e':
      if (!get_register(&reg))
	return false;
      if (!x_get_32_interval(&ry, &rz))
	return false;
      break;
    case '1':
    case '2':
    case '4':
      putchar_verb(c);
      len = (c - '0');
      if (VERBOSE)
	putstr("-byte addr:");
      if (!x_get_32(&address, 0))
	return false;
      if (!x_get_32_interval(&vy, &vz))
	return false;
      break;
    default:
      return false;
    }

  Jdb_bp::set_bpres(bpn, other, thread, reg, ry, rz, len, address, vy, vz);
  putchar_verb('\n');

  return true;
}

static 
void 
Jdb::show_breakpoint(int num)
{
  unsigned address, mode, len;

  printf("breakpoint %d: ", num+1);

  Jdb_bp::get_bp(num, &address, &mode, &len);

  if (address) 
    {
      printf("%s on %s at %08x", 
	     mode & 0x80 ? "LOG" : "BREAK",
	     bp_mode_names[mode & 3], address);
      if ((mode & 3) != 0)
	printf(" len %d", len);

      putchar('\n');
    }
  else
    puts("disabled");
}

static 
void 
Jdb::list_breakpoints()
{
  putchar('\n');

  if (bp_base) 
    printf("breakpoint base: %08x\n\n", bp_base);

  for(int i=0; i<4; i++)
    show_breakpoint(i);

  putchar('\n');
}

static 
void 
Jdb::list_breakpoint_restrictions(void)
{
  char reg;
  int other, thread;
  unsigned ry, rz, address, len, vy, vz;

  for (int i=0; i<4; i++) 
    {
      Jdb_bp::get_bpres(i, &other, &thread, 
			&reg, &ry, &rz, 
			&len, &address, &vy, &vz);

      printf("breakpoint %d: ", i+1);

      if ((thread == -1) && (reg == 0) && (len == 0))
	puts("not restricted");
      else
	{
	  int j = 0;
	  
	  putstr("restricted to ");
	  if (thread != -1)
	    {
	      j++;
	      printf("%s %x.%x\n",
		     other ? "thread !=" : "thread",
		     thread / L4_uid::threads_per_task(), 
		     thread % L4_uid::threads_per_task());
	    }
	  if (reg != 0)
	    {
	      if (j++)
		printf("%28s", "and ");
	      printf("register %s in [%08x, %08x]\n",
		  (reg > 0) && (reg < 10) ? reg_names[reg-1] : "???", ry, rz);
	    }
	  if (len != 0)
	    {
	      if (j++)
		printf("%28s", "and ");
	      printf("%d-byte var at %08x in [%08x, %08x]\n", 
		  len, address, vy, vz); 
	    }
	}
    }
  putchar('\n');
}

static 
int
Jdb::delete_breakpoint()
{
  int n, num, used;
  
  Jdb_bp::bps_used(&num, &used);

  if (!used)
    return false;
  
  if (VERBOSE)
    putstr(" number:");
  if ((n = get_num(used, 0)) == -1)
    return false;
  
  if (VERBOSE)
    printf("\ndeleting breakpoint %d\n", n);
  Jdb_bp::delete_bp(n-1);

  return true;
}

static
bool
Jdb::logmode_breakpoint(char trace_mode)
{
  int n, num, used;
  unsigned address, mode, len;
  
  Jdb_bp::bps_used(&num, &used);

  if (!used)
    return false;

  if (VERBOSE)
    putstr(" number:");
  if ((n = get_num(used, 0)) == -1)
    return false;

  Jdb_bp::get_bp(n-1, &address, &mode, &len);
  
  if (trace_mode == '+')
    mode &= ~0x80;
  else
    mode |= 0x80;

  putchar_verb('\n');

  Jdb_bp::set_bp(n-1, address, mode, len);

  if (VERBOSE)
    show_breakpoint(n-1);

  return true;
}

static
void
Jdb::set_breakpoints(trap_state *ts)
{
  unsigned mode = 0;
  switch (int c=getchar())
    {
    case 'a':
      mode = 3;
      // access breakpoint, fall through
    case 'w':
      mode |= 1;
      // write breakpoint, fall through
    case 'i':
      // mode == 0, instruction break point
      putchar_verb(c);
      if (!set_breakpoint(mode, ts))
	goto exit_error;
      break;
    case 'p':
      // I/O access
      mode = 2;
      putchar_verb(c);
      // check if machine supports hardware I/O BPs
      if (!(Cpu::features() & FEAT_DE)) 
	{
	  puts("I/O BPs not supported");
	  goto exit_error;
	}
      set_cr4(get_cr4() | CR4_DE);
      if (!set_breakpoint(mode, ts))
	goto exit_error;
      break;
    case 'b':
      // break point base
      // doesn't affect breakpoints already set
      putchar_verb(c);
      if (!x_get_32(&bp_base, 0))
	bp_base = 0;

      // algin breakpoint base to dword boundard
      bp_base &= ~3;
      putchar_verb('\n');
      break;
    case KEY_RETURN:
    case 'l':
    case ' ':
      putchar_verb(c);
      list_breakpoints();
      list_breakpoint_restrictions();
      break;
    case '-':
      putchar_verb(c);
      if (!delete_breakpoint())
	if (VERBOSE)
	  goto exit_error;
      break;
    case 'r':
      // restrict breakpoint
      putchar_verb(c);
      if (!restrict_breakpoint())
	goto exit_error;
      break;
    case '+':
    case '*':
      putchar_verb(c);
      if (!logmode_breakpoint(c))
	goto exit_error;
      break;
    case 't':
      putchar_verb(c);
      back_trace();
      break;
    }
  return;

exit_error:
  abort_command();
}

static 
void
Jdb::show_help(void)
{
  static const char * const help_message =
" GENERAL\n"
"   h|x|?                   show this help\n"
"   ^                       reboot\n"
"   Return                  show debug message\n"
"   B{lines}                show kdb output buffer\n"
"   V{a|h|s}                switch to vga/hercules/source level debugger\n"
"   C[x]                    set the prompt color to x, where x is a color:\n"
"                           nN: noir(black), rR: red, gG: green, bB: blue,\n"
"                           yY: yellow, mM: magenta, cC: cyan, wW: white;\n"
"                           the capital letters are for bold text.\n"
"   E{+|-}                  on/off enter kernel debugger by ESCape\n"
"   g                       leave kernel debugger\n"
"\n"
" INFO\n"
"   d[t{taskno}]{xxxxxxxx}  dump memory of given/current task at addr\n"
"   p[taskno|xxxxxxxx]      show pagetable of current/given task/addr\n"
"   t[taskno.threadno]      show current/given thread control block\n"
"   l{p|r}                  show present list/ready queue\n"
"   m{p|a}xxxxx             show mapping database starting at page/addr\n"
"#  r[taskno]               display IO bitmap of current/given task\n"
"   i|o{1|2|4|p[|a|m]}xxxx  in/out port, ack/(un)mask irq\n"
"   k{i|p|m|r|c}            show various kernel information (kh=help)\n"
"   bt[t|xxxxxxxx]          show backtrace of current/given thread/addr\n"
"   M{r|w}xxxxxxxx          read/write machine status register\n"
"   A{r|w}xxxxxxxx          read/write any physical address\n"
"\n"
" MONITORING\n"
"   P{+|-|*|R{+|-}}         on/off/buffer pagefault logging, on/off result\n"
"   Pr{t|T|x}               restrict pagefaults to thread/!thread/addr\n"
"   I{+|-|*|R{+|-}|T{+|-}}  on/off/buffer ipc logging, on/off result, tracing\n"
"   Ir{t|T|s}               restrict ipc's to thread/!thread/send-only\n"
"   T{P{+|-|k|u|xxxx}       enter tracebuffer, on/off/kernel/user perf\n"
#ifndef NO_LOG_EVENTS
"   O{x}{+|-}               on/off special logging event #x\n"
#endif
"\n"
" DEBUGGING\n"
"   S{+|-}                  on/off single step\n"
"   b{i|a|w|p}xxxxxxxx      set instruction/access/write/io breakpoint\n"
"   b{-}                    list breakpoints/delete one breakpoint\n"
"   br{num}{t|T|e|1|2|4}    restrict breakpoint to thread/register/addr\n"
"   br{num}-                delete breakpoint restrictions\n"
"   b{+|*}{num}             stop on breakpoint (default)/log event to buffer\n"
"#  u{t[taskno]}[xxxxxxxx]  disassemble bytes of given/current task addr\n"
"#  L                       show last branch recording information\n"
"\n";
  
  fancy_clear_screen();

  const char *c;
  int lines;
  
  for (c=help_message, lines=0; *c; c++)
    {
      if (*c == '\n')
	{
	  putchar('\n');
	  if (lines++ > 22)
	    {
	      putstr("--- CR: line, SPACE: page, ESC: abort ---");
	      int a=getchar();
	      printf("\r\033[K");
	      switch (a)
		{
		case 0x1b:
		case 'q':
		  putchar('\n');
		  return;
		case 0x0d:
		  lines--;
		  break;
		default:
		  lines=0;
		  break;
		}
	    }
	}
      else if (*c == '#' && *(c-1) == '\n')
	{
	  bool skip=false;
	  switch (*(c+3))
	    {
	    case 'u': skip = disasm_bytes==0; break;
	    case 'r': skip = Config::enable_io_protection==false; break;
	    case 'L': skip = Cpu::lbr_type()==Cpu::LBR_NONE; break;
	    }
	  if (skip)
	    {
	      while (*c!='\n')
		c++;
	    }
	  else
	    {
	      putchar(' ');
	    }
	}
      else
	{
	  putchar(*c);
	}
    }
}

static 
int
Jdb::switch_debug_state(trap_state *ts)
{
  putstr("\bswitch_debug_state");
  switch (int c=getchar()) 
    {
    case 'a':
      putchar('\n');
      return 1;

    case 's':
      // pass control to source level debugger
      if (nested_trap_handler) 
	{
	  putchar(c);
	  blink_cursor(LAST_LINE, 0); // the last thing to do
	  use_nested = 1;
	  gdb_trap_recover = 0;
	  if (use_nested)
    	    return nested_trap_handler(ts);
	  return 1;
	}
      puts("no source level debugger available");
      return 1;

    default:
      abort_command();
      return 1;
    }
}

static 
void 
Jdb::set_pagefault_protocol()
{
  switch (int c=putchar_verb(getchar()))
    {
    case '+': 
      Thread::log_page_fault = true;
      Thread::log_pf_to_buf  = false;
      break;
    case '*':
      Thread::log_page_fault = true;
      Thread::log_pf_to_buf  = true;
      break;
    case '-':
      Thread::log_page_fault = false;
      break;
    case ' ':
    case KEY_RETURN:
      break;
    case 'R':
      switch (c=getchar())
	{
	case '+':
	case '-':
	  Jdb_tbuf::log_events[LOG_EVENT_PF_RES]->enable(c=='+');
	  putchar_verb(c);
	  show_pagefault_protocol();
	  return;
	default:
	  abort_command();
	  return;
	}
      break;
    case 'r':
	{
	  int other, pfa_set;
	  GThread_num thread;
	  Address y, z;

	  Jdb_trace::get_pf_res(&other, &thread, &pfa_set, &y, &z);
	  switch (c=getchar())
	    {
	    case '-':
	      putchar_verb(c);
	      Jdb_trace::clear_pf_res();
	      show_pagefault_protocol();
	      return;
	    case 'T':
	    case 't':
	      if (VERBOSE)
      		printf(" %chread:",c);
	      if ((thread = get_thread_int()) == (GThread_num)-1)
		return;
	      other = (c == 'T');
	      break;
	    case 'x':
	      if (!x_get_32_interval(&y, &z))
		return;
	      pfa_set = 1;
	      break;
	    default:
	      abort_command();
	      return;
	    }
	  Jdb_trace::set_pf_res(other, thread, pfa_set, y, z);
	}
      break;
    default:
      abort_command();
      return;
    }
  show_pagefault_protocol();
}

static
void
Jdb::show_pagefault_protocol()
{
  if (VERBOSE)
    {
      putchar('\n');
      if (Thread::log_page_fault)
	{
	  int other, pfa_set;
	  GThread_num thread;
	  Address y, z;
	  
	  Jdb_trace::get_pf_res(&other, &thread, &pfa_set, &y, &z);
	  
	  printf("PF logging%s%s enabled",
	      Jdb_tbuf::log_events[LOG_EVENT_PF_RES]->enabled() 
		? " incl. results" : "",
		Thread::log_pf_to_buf ? " to tracebuffer" : "");
	  if (thread != (GThread_num)-1)
	    {
	      printf(", restricted to thread%s %x.%02x",
		     other ? "s !=" : "",
		     thread / L4_uid::threads_per_task(),
		     thread % L4_uid::threads_per_task());
	    }
	  if (pfa_set)
	    {
	      if (thread != (GThread_num)-1)
		putstr(" and ");
	      else
		putstr(", restricted to ");
	      if (y <= z)
		printf("%08x <= pfa <= %08x", y, z);
	      else
		printf("pfa < %08x || pfa > %08x", z, y);
	    }
	}
      else
	putstr("PF logging disabled");
      putchar('\n');
    }
}

static 
void 
Jdb::set_ipc_protocol()
{
  switch (int c=putchar_verb(getchar()))
    {
    case '+': 
      Thread::log_ipc = true;
      Thread::log_ipc_to_buf = false;
      Thread::set_ipc_vector();
      break;
    case '*':
      Thread::log_ipc = true;
      Thread::log_ipc_to_buf = true;
      Thread::set_ipc_vector();
      break;
    case '-':
      Thread::log_ipc = false;
      Thread::set_ipc_vector();
      break;
    case ' ':
    case KEY_RETURN:
      break;
    case 'R':
      switch (c=getchar())
	{
	case '+':
	case '-':
	  Thread::log_ipc_result = (c == '+');
	  putchar_verb(c);
	  show_ipc_protocol();
	  return;
	default:
	  abort_command();
	  return;
	}
      break;
    case 'T':  // IPC tracing
      switch (c=putchar_verb(getchar()))
        {
        case '+':
        case '*':
	  Thread::trace_ipc = true;
	  Thread::set_ipc_vector();
          break;
        case '-':
	  Thread::trace_ipc = false;
          Thread::set_ipc_vector();
          break;
        case ' ':
        case KEY_RETURN:
          break;
        default:
          abort_command();
          return;
        }
      break;
    case 'S':  // use the slow IPC path
      switch (c=putchar_verb(getchar()))
        {
        case '+':
        case '*':
	  Thread::slow_ipc = true;
	  Thread::set_ipc_vector();
          break;
        case '-':
	  Thread::slow_ipc = false;
          Thread::set_ipc_vector();
          break;
        case ' ':
        case KEY_RETURN:
          break;
        default:
          abort_command();
          return;
        }
      break;
    case 'r': 
	{
	  int other_thread, send_only;
	  GThread_num thread;
	  int other_task;
	  Task_num task;
	  
	  Jdb_trace::get_ipc_res(&other_thread, &thread, 
				 &other_task, &task, &send_only);
	  switch (c=getchar())
	    {
	    case '-':
	      putchar_verb(c);
	      Jdb_trace::clear_ipc_res();
	      show_ipc_protocol();
	      return;
	    case 'a':
	    case 'A':
	      if (VERBOSE)
		printf(" task%c=", (c == 'A') ? '!' : '=');
	      if ((task = get_task_int()) == (Task_num)-1)
		return;
	      other_task = (c == 'A');
	      break;
	    case 'T':
	    case 't':
	      if (VERBOSE)
      		printf(" thread%c=", (c == 'T') ? '!' : '=');
	      if ((thread = get_thread_int()) == (GThread_num)-1)
		return;
	      other_thread = (c == 'T');
	      break;
	    case 's':
	      send_only = 1;
	      break;
	    default:
	      abort_command();
	      return;
	    }
	  Jdb_trace::set_ipc_res(other_thread, thread,
				 other_task, task, send_only);
	  break;
	}
    default:
      abort_command();
      return;
    }
  show_ipc_protocol();
}

static
void
Jdb::show_ipc_protocol()
{
  if (VERBOSE)
    {
      putchar('\n');
      if (Thread::trace_ipc)
	putstr("IPC tracing enabled");
      else if (Thread::log_ipc)
	{
	  int other_thread, send_only;
	  GThread_num thread;
	  int other_task;
	  Task_num task;
	  
	  Jdb_trace::get_ipc_res(&other_thread, &thread, 
				 &other_task, &task, &send_only);
	  
	  printf("IPC logging%s%s enabled",
	      Thread::log_ipc_result ? " incl. results" : "",
	      Thread::log_ipc_to_buf ? " to tracebuffer" : "");
	  if (thread != (GThread_num)-1)
	    {
	      printf("\n    restricted to thread%s %x.%02x%s",
		     other_thread ? "s !=" : "",
		     thread / L4_uid::threads_per_task(),
		     thread % L4_uid::threads_per_task(),
		     send_only ? ", send-only" : "");
	    }
	  if (task != (Task_num)-1)
	    {
	      printf("\n    restricted to task%s %x",
		     other_task ? "s !=" : "", task);
	    }
	}
      else if (Thread::slow_ipc)
	putstr("IPC logging disabled / using the IPC slowpath");
      else
	putstr("IPC logging disabled / using the IPC fastpath");

      putchar('\n');
    }
}

static 
void 
Jdb::set_unmap_protocol()
{
  switch (int c=putchar_verb(getchar()))
    {
    case '+':
      Thread::log_unmap = true;
      Thread::log_unmap_to_buf = false;
      Thread::set_unmap_vector();
      break;
    case '*':
      Thread::log_unmap = true;
      Thread::log_unmap_to_buf = true;
      Thread::set_unmap_vector();
      break;
    case '-':
      Thread::log_unmap = false;
      Thread::set_unmap_vector();
      break;
    case ' ':
    case KEY_RETURN:
      break;
    case 'r':
	{
	  int other, addr_set;
	  GThread_num thread;
	  Address y, z;

	  Jdb_trace::get_unmap_res(&other, &thread, &addr_set, &y, &z);
	  switch (c=getchar())
	    {
	    case '-':
	      putchar_verb(c);
	      Jdb_trace::clear_unmap_res();
	      show_unmap_protocol();
	      return;
	    case 'T':
	    case 't':
	      if (VERBOSE)
      		printf(" %chread:",c);
	      if ((thread = get_thread_int()) == (GThread_num)-1)
		return;
	      other = (c == 'T');
	      break;
	    case 'x':
	      if (!x_get_32_interval(&y, &z))
		return;
	      addr_set = 1;
	      break;
	    default:
	      abort_command();
	      return;
	    }
	  Jdb_trace::set_unmap_res(other, thread, addr_set, y, z);
	}
      break;
    default:
      abort_command();
      return;
    }
  show_unmap_protocol();
}

static
void
Jdb::show_unmap_protocol()
{
  if (VERBOSE)
    {
      putchar('\n');
      if (Thread::log_unmap)
	{
	  int other, addr_set;
	  GThread_num thread;
	  Address y, z;
	  
	  Jdb_trace::get_unmap_res(&other, &thread, &addr_set, &y, &z);
	  
	  printf("UNMAP logging%s enabled",
	      Thread::log_unmap_to_buf ? " to tracebuffer" : "");
	  if (thread != (GThread_num)-1)
	    {
	      printf(", restricted to thread%s %x.%02x",
		     other ? "s !=" : "",
		     thread / L4_uid::threads_per_task(), 
		     thread % L4_uid::threads_per_task());
	    }
	  if (addr_set)
	    {
	      if (thread != (GThread_num)-1)
		putstr(" and ");
	      else
		putstr(", restricted to ");
	      if (y <= z)
		printf("%08x <= addr <= %08x", y, z);
	      else
		printf("addr < %08x || addr > %08x", z, y);
	    }
	}
      else
	putstr("UNMAP logging disabled");
      putchar('\n');
    }
}

static
bool
Jdb::show_pages(trap_state *ts, int first_char=0)
{
  unsigned task = specified_task;
  unsigned address;
  Address ptab;
  
  if (!x_get_32(&address, 0, first_char)) 
    {
      if (address != 0)
	return false;
      
      ptab = (Address)Space_context::current();
    }
  else if (address < 2048)  // interpret as task number
    {
      task = address;
      if ((address = task_lookup(task)) == PAGE_INVALID)
	{
	  puts(" Invalid task");
	  return false;
	}

      ptab = (Address)Kmem::phys_to_virt(address);
    }
  else
    // else interpret address as ptab address
    ptab = address;

  return dump_address_space(ts, PAGE_DIR, ptab, task);
}

static
bool
Jdb::show_memory(trap_state *ts)
{
  unsigned task;
  Address address;

  if (!get_task_address(&address, &task, 0, ts))
    return false;
  
  return dump_address_space(ts, D_MODE, address, task);
}

static 
void 
dummy(void) 
{
  static int dummy_read, tmp, dummy_write;
  tmp=dummy_read;
  asm("nop;nop;nop;nop;");
  dummy_write=tmp;
}

static 
void 
Jdb::esc_key()
{
  switch (int c=getchar()) 
    {
    case '+':
    case '-':
      Config::esc_hack = (c == '+');
      putchar_verb(c);
      putchar_verb('\n');
      break;
    default:
      abort_command();
      break;
    }
}

static
void
Jdb::list_threads_show_thread(Thread *t)
{
  char waitfor[24], to[24];
  
  *waitfor = *to = '\0';

  if (!dump_gzip)
    getchar_chance();

  if (t->partner()
      && (t->state() & (  Thread_receiving
			| Thread_busy
			| Thread_rcvlong_in_progress)))
    {
      if (   t->partner()->_id.is_irq())
	sprintf(waitfor, "w:irq %02x",
	    t->partner()->_id.irq());
      else
	sprintf(waitfor, "w:%3x.%02x",
	    t->partner()->_id.task(), 
	    t->partner()->_id.lthread());
    }
  else if (t->state() & Thread_waiting)
    {
      strcpy(waitfor, "w:  *.**");
    }

  if (*waitfor)
    {
      if (t->_timeout && t->_timeout->is_set())
	{
	  Unsigned64 diff = (t->_timeout->get_timeout());
	  if (diff >= 100000000LL)
	    strcpy(to, "  to: >99s");
	  else
	    {
	      int us = (int)diff;
	      if (us < 0)
		us = 0;
	      if (us >= 1000000)
		sprintf(to, "  to: %3us", us / 1000000);
	      else if (us >= 1000)
		sprintf(to, "  to: %3um", us / 1000);
	      else
		sprintf(to, "  to: %3u\346", us);
	    }
	}
    }
  
 
  if (dump_gzip)
    {
      char buf[90];
      sprintf(buf, "%3x.%02x  p:%2x  %8s%-10s  s:",
       	  t->_id.task(),  t->_id.lthread(),
	  t->sched()->prio(), waitfor, to);
      gz_write(buf, strlen(buf));
      print_thread_state(t, 50);
      gz_write("\n", 1);
    }
  else
    {
      printf("%3x.%02x  p:%2x  %8s%-10s  s:",
	  t->_id.task(),  t->_id.lthread(),
	  t->sched()->prio(), waitfor, to);

      print_thread_state(t, 42);
      clear_to_eol();
      putchar('\n');
    }
}

static
void
Jdb::list_threads(trap_state *ts, char pr)
{
  unsigned y, y_max;
  Thread *t, *t_current = current_active;

  // sanity check
  if (!check_thread(t_current))
    {
      printf(" No threads\n");
      return;
    }

  // make sure that we have a valid starting point
  if ((pr=='r') && (!t_current->in_ready_list()))
    t_current = kernel_thread;

  fancy_clear_screen();

  jdb_thread_list::init(pr, t_current);
  
  for (;;)
    {
      jdb_thread_list::set_start(t_current);

      // set y to position of t_current in current displayed list
      y = jdb_thread_list::lookup(t_current);

      for (bool resync=false; !resync;)
	{
	  // display 24 lines
	  home();
	  y_max = jdb_thread_list::page_show(list_threads_show_thread);

	  // clear rest of screen (if where less than 24 lines)
	  for (int i=y_max; i < 23; i++)
	    {
	      clear_to_eol();
	      putchar('\n');
	    }
	  
	  printf_statline("l%c %-15s%56s", pr, jdb_thread_list::get_mode_str(),
	      "KEYS: Up Dn PgUp PgDn Space Tab Home End CR");
	  
	  // key event loop
	  for (bool redraw=false; !redraw; )
	    {
	      cursor(y, 5);
	      switch (int c=getchar())
		{
		case KEY_CURSOR_UP:
		  if (y > 0)
		    y--;
		  else
		    redraw = jdb_thread_list::line_back();
		  break;
		case KEY_CURSOR_DOWN:
		  if (y < y_max)
		    y++;
		  else
		    redraw = jdb_thread_list::line_forw();
		  break;
		case KEY_PAGE_UP:
		  if (!(redraw = jdb_thread_list::page_back()))
		    y = 0;
		  break;
		case KEY_PAGE_DOWN:
		  if (!(redraw = jdb_thread_list::page_forw()))
		    y = y_max;
		  break;
		case KEY_CURSOR_HOME:
		  redraw = jdb_thread_list::goto_home();
		  y = 0;
		  break;
		case KEY_CURSOR_END:
		  redraw = jdb_thread_list::goto_end();
		  y = y_max;
		  break;
		case ' ': // switch mode
		  t_current = jdb_thread_list::index(y);
		  jdb_thread_list::switch_mode();
		  redraw = true;
		  resync = true;
		  break;
		case KEY_TAB: // goto thread we are waiting for
		  t = jdb_thread_list::index(y);
		  if (t->partner()
		      && (t->state() & (  Thread_receiving
					| Thread_busy
					| Thread_rcvlong_in_progress))
		      && (   (!t->partner()->_id.is_irq())
		          || (t->partner()->_id.irq() > Config::MAX_NUM_IRQ)))
		    {
		      t_current = static_cast<Thread*>(t->partner());
		      redraw = true;
		      resync = true;
		    }
		  break;
		case KEY_RETURN: // show current tcb
		  t = jdb_thread_list::index(y);
		  if (!(show_tcb(ts, t->_id, 1)))
		    return;
		  redraw = 1;
		  break;
		case KEY_ESC:
		  abort_command();
		  return;
		default:
		  if (is_toplevel_cmd(c)) 
		    return;
		}
	    }
	}
    }
}

static
void
Jdb::show_thread_list(trap_state *ts)
{
  getchar(); // ignore r/p

  Thread *t = get_thread(ts);

  // sanity check
  if (!check_thread(t))
    {
      printf(" No threads\n");
      return;
    }

  if (gz_init_done)
    {
      printf("\n\n=== start of thread list ===\n");
      dump_gzip = true;
      gz_open("threads.gz");
      jdb_thread_list::init('p', t);
      jdb_thread_list::set_start(t);
      jdb_thread_list::goto_home();
      jdb_thread_list::complete_show(list_threads_show_thread);
      gz_close();
      printf("=== end of thread list ===\n\n");
      dump_gzip = false;
    }
}

// mail loop
static
int
Jdb::execute_command(trap_state *ts)
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

      if (!next_auto_tcb)
	printf("\033[K%c", c);

      char _cmd[] = {c,0};
      Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

      if (cmd.cmd)
	{
	  int ret;

	  if (!(ret = Jdb_core::exec_cmd (cmd)))
	    {
	      hide_statline = false;
	      last_cmd = 0;
	    }

	  // show old help screen
	  if(c!='h')
	    return ret;

	  {
	    putstr("--- CR: line, SPACE: page, ESC: abort ---");
	    int a=getchar();
	    printf("\r\033[K");
	    switch (a)
	      {
	      case 0x1b:
	      case 'q':
		putchar('\n');
		return 1;
	      default:
		break;
	      }
	  }
	}

      switch(c)
	{
	case KEY_RETURN: // show debug message
	  hide_statline = false;
	  break;
	case '_':
	case '?':
	case 'h':
	case 'x': // show help page
	  show_help();
	  break;
	case 't': // show thread control block
	  if (next_auto_tcb)
	    {
	      show_tcb(ts, L4_uid::INVALID);
	      next_auto_tcb = false;
	    }
	  else
	    show_tcb_cmd(ts);
	  break;
	case 'S': // enable/disable single step
	  set_single_step(ts);
	  break;
	case 'P': // enable/disable/buffer/restrict pagefaults
	  set_pagefault_protocol();
	  break;
	case 'B': // print last n lines of output buffer
	  c = get_value(12, DEC_VALUE);
	  putchar('\n');
	  console_buffer()->print_buffer(c);
	  break;
	case 'L': // print registers of last branch recording MSR's
	  show_lbr();
	  break;
	case 'O':
	  set_log_event();
	  break;
	case 'I': // enable/disable/buffer/restrict ipc's
	  set_ipc_protocol();
	  break;
	case 'U':
	  set_unmap_protocol();
	  break;
	case 'b': // set/reset/restrict breakpointer or do backtrace
	  set_breakpoints(ts);
	  break;
	case 'p': // show pagemap
      	  show_pages(ts, c);
	  break;
	case 'd': // dump memory
	  show_memory(ts);
	  break;
	case 'm': // show mapping tree
	  show_mapping();
	  break;
	case 'i': // read from i/o port
	  do_io_in();
	  break;
	case 'o': // write to i/o port
	  do_io_out();
	  break;
	case 's': // search in physical memory
	  search_cmd(ts);
	  break;  
	case 'T': // show tracebuffer
	  show_tracebuffer();
	  break;
	case 'V': // switch console to hercules/color/gdb
	  return switch_debug_state(ts);
	case 'E': // enable/disable "enter_kdebugger on <ESC>"
	  esc_key();
	  break;
	case 'u': // disassemble memory
	  // check if disasm module is linked into binary
	  if (disasm_bytes != 0)
	    show_disasm(ts);
	  else
	    backspace_verb();
	  break;
	case 'l': // list thread in ready queue / present list
	  switch (putchar(c=getchar()))
	    {
	    case 'p': // present list
	    case 'r': // ready queue
	      list_threads(ts, c);
	      break;
	    }
	  break;
	case 'r': // print IO port info
	  show_io_cmd();
	  break;
	case 'j': // do restricted "go"
	  switch (putchar(c=getchar()))
	    {
	    case 'b': // go until next branch
	    case 'r': // go until current function returns
	      ss_level = 0;
    	      if (code_call)
		{
	     	  // increase call level because currently we
		  // stay on a call instruction
    		  ss_level++;
		}
	      ss_state = (c == 'b') ? SS_BRANCH : SS_RETURN;
	      // if we have lbr feature, the processor treats the single
	      // step flag as step on branches instead of step on instruction
	      if (Cpu::lbr_type() == Cpu::LBR_P6)
		Cpu::wrmsr(2, 0x1d9);
	      // fall through
	    case 's': // do one single step
	      ts->eflags |= EFLAGS_TF;
	      hide_statline = false;
	      return 0;
	    default:
	      abort_command();
	      break;
	    }
	  break;
	case 'M': // set MSR or print content of MSR
	  get_set_msr();
	  break;
	case 'A':
	  get_set_adapter_memory();
	  break;
	case 'g': // leave kernel debugger, continue current task
	  hide_statline = false;
	  last_cmd = 0;
	  return 0;
	case 'q': // nothing
	  dummy();
	  break;
	case '^': // (qwerty: shift-6) do reboot
 	  putchar('\n');
	  serial_setscroll(1,127);
	  blink_cursor(LAST_LINE, 0);
	  serial_end_of_screen();
	  
	  terminate(1);
	default:
	  backspace_verb();
	  // ignore character and get next input
	  break;
	}
      last_cmd = c;
      return 1;
    }
}

// Interprete str as control sequences for jdb
// We only allow mostly non-interactive commands here which make sense 
// (e.g. we don't allow d, t, l, u commands)
PUBLIC static
int
Jdb::interprete_debug_command(trap_state *ts, const char *str, int len)
{
  // disable all interrupts
  open_debug_console();
  
  // setup getchar() so it reads from debug_ctrl_str instead of
  // reading keystrokes from keyboard until debug_ctrl_len is 0.
  debug_ctrl_str = str;
  debug_ctrl_len = len;

  was_error = false;

  get_current(ts);
  
  for (;;)
    {
      if (was_error)
	{
	  debug_ctrl_str = 0;
	  close_debug_console();
	  return false;
	}
      
      switch (getchar())
	{
	case KEY_RETURN: // end of string
	  debug_ctrl_str = 0;
	  close_debug_console();
	  return true;
	case 'b': // set/reset/restrict breakpointer or do backtrace
	  set_breakpoints(ts);
	  break;
	case 'E': // enable/disable "enter_kdebugger on <ESC>" E{+|-}
	  esc_key();
	  break;
	case 'l':
	  show_thread_list(ts);
	  break;
	case 'I': // enable/disable/buffer/restrict ipc's: I{+|*|-|r}
	  set_ipc_protocol();
	  break;
	case 'O':
	  set_log_event();
	  break;
	case 'P': // enable/disable/buffer/restrict pagefaults: P{+|*|-|r}
	  set_pagefault_protocol();
	  break;
	case 'S': // enable/disable single step (S{+|-})
	  set_single_step(ts);
	  break;
	case '^': // (qwerty: shift-6) do reboot
	  terminate(1);
	default:
	  abort_command();
    	}
    }
}

// take a look at the code of the current thread eip
// set global indicators code_call, code_ret, code_bra, code_int
// This can fail if the current page is still not mapped
static
void
Jdb::analyze_code(trap_state *ts)
{
  // do nothing if page not mapped into this address space
  Space *s = current_space();
  if (  (s->virt_to_phys(ts->eip)   == PAGE_INVALID)
      ||(s->virt_to_phys(ts->eip+1) == PAGE_INVALID))
    return;
  
  unsigned char op1, op2=0;
  
  op1 = *reinterpret_cast<char*>(ts->eip);
  if (op1 == 0x0f || op1 == 0xff)
    op2 = *reinterpret_cast<char*>(ts->eip+1);

  code_ret =  (   ((op1 & 0xf6) == 0xc2)	// ret/lret /xxxx
	       || (op1 == 0xcf));		// iret

  code_call = (   (op1 == 0xe8)			// call near
	       || ((op1 == 0xff)
	     	 && ((op2 & 0x30) == 0x10))	// call/lcall *(...)
	       || (op1 == 0x9a));		// lcall xxxx:xxxx

  code_bra =  (   ((op1 & 0xfc) == 0xe0)	// loop/jecxz
	       || ((op1 & 0xf0) == 0x70)	// jxx rel 8 bit
	       || (op1 == 0xeb)			// jmp rel 8 bit
	       || (op1 == 0xe9)			// jmp rel 16/32 bit
	       || ((op1 == 0x0f)
		 && ((op2 & 0xf0) == 0x80))	// jxx rel 16/32 bit
	       || ((op1 == 0xff)
		 && ((op2 & 0x30) == 0x20))	// jmp/ljmp *(...)
	       || (op1 == 0xea));		// ljmp xxxx:xxxx

  code_int =  (   (op1 == 0xcc)			// int3
	       || (op1 == 0xcd)			// int xx
	       || (op1 == 0xce));		// into
}

static
void
Jdb::get_current(trap_state *ts)
{
  current_active = get_thread(ts);
  
  if (Kmem::virt_to_phys(current_active) != PAGE_INVALID)
    {
      current_tid    = current_active->_id;
      specified_task = current_tid.task();
    }
  else
    {
      current_tid    = L4_uid::INVALID;
      specified_task = 0;
    }
}

// entered debugger because of single step trap
static inline
bool
Jdb::handle_single_step(trap_state *, char *error_buffer)
{
  bool really_break = true;

  // special single_step ('j' command): go until branch/return
  if (ss_state != SS_NONE)
    {
      if (Cpu::lbr_type() != Cpu::LBR_NONE)
	{
	  // don't worry, the CPU always knows what she is doing :-)
       	}
      else
	{
	  // we have to emulate lbr looking at the code ...
	  switch (ss_state)
	    {
	    case SS_RETURN:
	      // go until function return
	      really_break = false;
	      if (code_call)
		{
		  // increase call level
		  ss_level++;
		}
	      else if (code_ret)
		{
		  // decrease call level
		  really_break = (ss_level-- == 0);
		}
	      break;
	    case SS_BRANCH:
	    default:
	      // go until next branch
	      really_break = (code_ret || code_call || code_bra || code_int);
	      break;
	    }
	}

      if (really_break)
	{
	  // condition met
	  ss_state = SS_NONE;
	  strcpy(error_buffer, "Branch/Call");
	}
    }
  else // (ss_state == SS_NONE)
    // regular single_step
    strcpy(error_buffer, "Singlestep");

  return really_break;
}

// entered debugger because of met hardware breakpoint
static inline
bool
Jdb::handle_break_point(trap_state *ts, char *error_buffer)
{
  Thread *t = get_thread(ts);

  for (int i=0; i<4; i++)
    {
      if (db7 & (1<<i))
	{
	  bool really_break = !Jdb_bp::check_bpres(ts, i, t);
	  unsigned address, mode, len;

	  Jdb_bp::get_bp(i, &address, &mode, &len);

	  if (really_break)
    	    {
	      printf("really\n");
	      // really stop (enter command prompt)
    	      sprintf(error_buffer, "break on %s at %08x",
				  bp_mode_names[mode], address);
	      if (mode==1 || mode==3)
		{
	     	  // If it's a write or access (read) breakpoint,
		  // we look at the appropriate place and print
    		  // the bytes we find there. We do not need to
		  // look if the page is present because the x86
		  // enters the debug exception _after_ the memory
		  // access was performed.
		  char *msgptr = error_buffer + strlen(error_buffer);
		  switch (len)
		    {
		    case 1:
		      sprintf(msgptr, " [%02x]", *(Unsigned8*)address);
		      break;
		    case 2:
		      sprintf(msgptr, " [%04x]", *(Unsigned16*)address);
		      break;
		    case 4:
		      sprintf(msgptr, " [%08x]", *(Unsigned32*)address);
		      break;
		    }
		}
	      // else breakpoint restriction not met
	    }

	  // ensure that we don't break a second time at the same
	  // instruction breakpoint
	  if ((mode & 3) == 0)
    	    ts->eflags |= EFLAGS_RF;

	  return really_break;
	}
    }

  return true;
}

// entered debugger due to debug exception
static
bool
Jdb::handle_trap_1(trap_state *ts, char *error_buffer)
{
  Jdb_bp::reset_debug_status_register();

  if (db7 & SINGLE_STEP)
    {
      // the single step mode is the highest-priority debug exception
      return handle_single_step(ts, error_buffer);
    }
  else if (db7 & DEBREG_ACCESS)
    {
      // read/write from/to debug register
      sprintf(error_buffer, "debug register access");
      return true;
    }
  else if (db7 & 0xf)
    {
      // hardware breakpoint
      return handle_break_point(ts, error_buffer);
    }
  else
    {
      // other unknown reason
      sprintf(error_buffer, "unknown debug exception (db7=%08x)", db7);
      return true;
    }
}

// entered debugger due to software breakpoint
static inline
bool
Jdb::handle_trap_3(trap_state *ts, char *error_buffer)
{
  unsigned len = *reinterpret_cast<char*>(ts->eip + 1);
  const char *msg = reinterpret_cast<char*>(ts->eip + 2);

  if (*reinterpret_cast<unsigned char*>(ts->eip) == 0xeb) 
    {
      // we are entering here because enter_kdebugger("*#..."); failed
      if ((len > 1) && (msg[0] == '*') && (msg[1] == '#'))
	{
     	  unsigned i;
	  char ctrl[29];
	  len-=2;
	  msg+=2;
	  for (i=0; (i<sizeof(ctrl)-1) && (i<len); i++)
	    ctrl[i] = msg[i];
	  ctrl[i] = '\0';
	  sprintf(error_buffer, "invalid ctrl sequence \"%s\"", ctrl);
	}
      // enter_kdebugger("...");
      else if (len > 0)
	{
	  unsigned i;
	  len = len < 47 ? len : 47;
	  for(i=0; i<len; i++)
	    error_buffer[i] = *(msg+i);
	  error_buffer[i]='\0';
	}
    }
  else
    strcpy(error_buffer, "INT 3");

  return true;
}

// entered debugger due to other exception
static inline
bool
Jdb::handle_trap_x(trap_state *ts, char *error_buffer)
{
  if (traps[ts->trapno].error_code)
    sprintf(error_buffer, "%s (ERR=%08x)",
	traps[ts->trapno].name, ts->err);
  else 
    sprintf(error_buffer, "%s",
	traps[ts->trapno].name);

  return true;
}

static inline
int
Jdb::handle_int3(trap_state *ts)
{
  if(ts->trapno==3) 
    {
      unsigned char todo = *reinterpret_cast<char*>(ts->eip);

      switch (todo)
	{
	case 0xeb:		// jmp == enter_kdebug()
	  {
	    char len = *reinterpret_cast<char*>(ts->eip + 1);
	    char *str = reinterpret_cast<char*>(ts->eip + 2);
	    if ((len > 1) && (str[0] == '*') && (str[1] == '#'))
	      {
    		// jdb command
		if (Jdb::interprete_debug_command(ts, str+2, len-2))
		  return 1;
	      }
	  }
	  break;

	case 0x3c: // cmpb
	  todo = *reinterpret_cast<char*>(ts->eip + 1);
	  switch (todo)
	    {
	    case 29:
	      switch (ts->eax)
		{
		case 2:
		  Jdb_tbuf::clear_tbuf();
		  show_tb_absy = 0;
		  show_tb_refy = 0;
		  show_tb_nr   = (Mword)-1;
		  return 1;
		case 3:
		  Watchdog::disable();
		  dump_gzip = true;
		  printf("\n\n=== start of trace ===\n");
		  Jdb::show_tracebuffer_events(0, 0, 1000000, 0, 0);
		  printf("=== end of trace ===\n\n");
		  dump_gzip = false;
		  Watchdog::enable();
		  return 1;
		}
	      break;
	    case 30:		// register debug symbols/lines information
	      switch (ts->ecx)
		{
		case 1:
		  Jdb_symbol::register_symbols(ts->eax, ts->ebx);
		  return 1;
		case 2:
		  Jdb_lines::register_lines(ts->eax, ts->ebx);
		  return 1;
		case 3:
#ifdef CONFIG_APIC_MASK
		  apic_irq_nr = ts->eax;
		  printf("Special handling for APIC interrupt 0x%02x\n",
			 apic_irq_nr);
#else
		  panic("CONFIG_APIC_MASK not defined");
#endif
		  return 1;
		}
	    }
	  break;
	}
    }
  return 0;
}

static void serial_setscroll(int begin, int end)
{
  printf("\033[%d;%dr",begin,end);
}

static void serial_end_of_screen()
{
  putstr("\033[127;1H");
}

static inline
void
Jdb::enable_lbr()
{
  if (lbr_active)
    Cpu::enable_lbr();
}

PUBLIC static
int
Jdb::enter_kdebugger(trap_state *ts)
{
  Cpu::disable_lbr();

  // check for int $3 user debugging interface
  if (handle_int3(ts))
    return 0;

  volatile bool really_break = true;

  static jmp_buf recover_buf;
  static char error_buffer[81];

  if (use_nested && nested_trap_handler)
    // switch to gdb
    return nested_trap_handler(ts);

  if (ts->trapno != 1)
    {
      // don't enter kdebug if user pressed a non-debug command key
      int c = Kconsole::console()->getchar(false);
      if (   (c != -1)		/* valid input */
	  && (c != 0x20)	/* <SPACE> */
	  && (c != 0x1b)	/* <ESC> */
	  && (!is_toplevel_cmd(c)))
	return 0;
    }

  gdb_trap_eip = ts->eip;
  gdb_trap_cr2 = ts->cr2;
  gdb_trap_no  = ts->trapno;

  if (gdb_trap_recover)
    {
      // re-enable interrupts if we need them
      if (Config::getchar_does_hlt)
	sti();
      longjmp(recover_buf, 1);
    }

  db7 = Jdb_bp::get_debug_status_register();

  if ((ts->trapno == 1)
      && Jdb_bp::check_bp_log(ts, db7, get_thread(ts)))
    {
      enable_lbr();
      return 0;
    }

  console_buffer()->disable();
  // disable all interrupts
  open_debug_console();
  hide_statline = false;

  // Update page directory to latest version
  Thread *t = Thread::lookup(context_of(ts));
  if (check_thread(t))
    {
      for (vm_offset_t address = Kmem::mem_tcbs;
	   address < Kmem::mem_tcbs 
		     + Context::size * L4_uid::threads_per_task() * 2048;
	   address += Config::SUPERPAGE_SIZE)
	{
	  t->space()->kmem_update (address);
	}
    }
  
  // look at code (necessary for 'j' command)
  analyze_code(ts);

  // clear error message
  *error_buffer = '\0';
 
  if (ts->trapno == 1)
    really_break = handle_trap_1(ts, error_buffer);
  else if (ts->trapno == 3)
    really_break = handle_trap_3(ts, error_buffer);
  else if (ts->trapno < 20)
    really_break = handle_trap_x(ts, error_buffer);

  if (really_break)
    {
      if (!single_step_cmd)
	ts->eflags &= ~EFLAGS_TF;
      // else S+ mode
      
      if (auto_tcb)
      	{
	  next_cmd = 't';
	  next_auto_tcb = 1;
	  hide_statline = true;
	  // clear any keystrokes in queue because later we enter show_tcb
	  while (Kconsole::console()->getchar(false) != -1)
	    ;
	}
    }

  while (setjmp(recover_buf))
    {
      // handle traps which occured while we are in Jdb
      switch (gdb_trap_no)
	{
	case 2:
	  cursor(LAST_LINE, 0);
	  printf("\nNMI occured\n");
	  break;
	case 3:
	  cursor(LAST_LINE, 0);
	  printf("\nSoftware breakpoint inside jdb at %08x\n", 
	      gdb_trap_eip-1);
	  break;
	case 13:
	  if (test_msr)
	    {
	      printf(" MSR does not exist or invalid value\n");
	      test_msr = 0;
	    }
	  else
	    {
	      cursor(LAST_LINE, 0);
	      printf("\nGeneral Protection (eip=%08x) -- jdb bug?\n",
		  gdb_trap_eip);
	    }
	  break;
	default:
	  cursor(LAST_LINE, 0);
	  printf("\nInvalid access (trap=%02x addr=%08x eip=%08x) "
	         "-- jdb bug?\n",
	      gdb_trap_no, gdb_trap_cr2, gdb_trap_eip);
	  break;
	}
    }

  if (really_break) 
    {
      get_current(ts);

      do 
	{
	  serial_setscroll(1, 25);
	  if (!hide_statline) 
	    {
	      cursor(24, 0);
	      putstr(prompt_esc);
	      printf("\n%s"
		     "    "
		     "--------------------------------------------------------"
		     "EIP: %08x\033[m      \n", 
		     Boot_info::get_checksum_ro() != checksum::get_checksum_ro()
			? "    WARNING: Fiasco kernel checksum differs -- "
			  "read-only data has changed!\n"
			: "",		     
		     ts->eip);
	      cursor(23, 6);
	      printf("%s", error_buffer);
	      hide_statline = true;
	    }

	  gdb_trap_recover = 1;
	  printf_statline(0);

	} while (execute_command(ts));

      // next time we do show_tracebuffer start listing at last entry
      show_tb_absy = 0;
      show_tb_refy = 0;
      
      // scroll one line
      putchar('\n');
      
      // reset scrolling region of serial terminal
      serial_setscroll(1,127);
      
      // reset cursor
      blink_cursor(LAST_LINE, 0);
      
      // goto end of screen
      serial_end_of_screen();
      console_buffer()->enable();
    }
 
  // reenable interrupts
  close_debug_console();
  gdb_trap_recover = 0;

  if(Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::uart()->enable_rcv_irq();

  enable_lbr();
  
  return 0;
}

// Do page lookups for other tasks.
// Interface for superpages: bool superpage
// Look if a page exists for this address,
// if page is valid, return it, else return 0xffffffff
static 
unsigned
Jdb::page_lookup(unsigned va, unsigned task, bool *superpage, unsigned *offset)
{
  vm_offset_t top_a;

  if ((top_a = task_lookup(task)) == PAGE_INVALID)
    return PAGE_INVALID;

  pd_entry_t *pdir = pdir_find_pde(top_a, 0);

  // get index into table
  unsigned pdn = (va >> PDESHIFT) & PDEMASK;

  if (!(pdir[pdn] & INTEL_PDE_VALID)) 
    return PAGE_INVALID;
  
  // PDIR is OK, now check PTAB
  if (pdir[pdn] & INTEL_PDE_SUPERPAGE)
    {
      *superpage = true;
      *offset = va & ~Config::SUPERPAGE_MASK;
      return pde_to_pa(pdir[pdn]); // return superpage address
    }

   unsigned ptn = (va >> PTESHIFT) & PTEMASK;
   
   pt_entry_t *ptab = ptab_find_pte(pde_to_pa(pdir[pdn]), 0);
   
   if (!(ptab[ptn] & INTEL_PTE_VALID)) 
     return PAGE_INVALID; 

   // PTAB is OK, return physical address of page
   *superpage = false;
   *offset = va & ~Config::PAGE_MASK;
   return pte_to_pa(ptab[ptn]); 
}

// return virtual address inside kernel of virtual address of a task
static
unsigned
Jdb::k_lookup(unsigned va, unsigned task)
{
  unsigned a, p;
  bool dummy_superpage;

  if ((p = page_lookup(va, task, &dummy_superpage, &a)) == PAGE_INVALID)
    return p;

  return (unsigned)(Kmem::phys_to_virt(p + a));
}

static
int
Jdb::get_pci_bus_dev_reg(unsigned *bus, unsigned *dev, unsigned *reg)
{
  printf("pci config dword - bus:");
  if ((*bus = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;
  printf(" dev:");
  if ((*dev = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;
  printf(" reg:");
  if ((*reg = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;

  return true;
}

PUBLIC static 
int Jdb::set_prompt_color(char x)
{
  unsigned pc = 32;
  unsigned ph = 0;
 
  switch(x) 
    {
    case 'N': ph = 1;
    case 'n': pc = 30; break;

    case 'R': ph = 1;
    case 'r': pc = 31; break;

    case 'G': ph = 1;
    case 'g': pc = 32; break;
      
    case 'Y': ph = 1;
    case 'y': pc = 33; break;

    case 'B': ph = 1;
    case 'b': pc = 34; break;

    case 'M': ph = 1; 
    case 'm': pc = 35; break;

    case 'C': ph = 1;
    case 'c': pc = 36; break;

    case 'W': ph = 1;
    case 'w': pc = 37; break;
      
    default:
      return 0;
    }

  if(ph>0)
    snprintf(prompt_esc,sizeof(prompt_esc)-1,"\033[%d;%dm",pc,ph);
  else
    snprintf(prompt_esc,sizeof(prompt_esc)-1,"\033[%dm",pc);

  return 1;
}


static 
void
Jdb::do_io_in(void)
{
  unsigned port, answer = 0xffffffff;
  unsigned bus, dev, reg;

  printf("n ");
  
  int porttype=getchar();
  switch (porttype)
    {
    case 'p':
      if (!get_pci_bus_dev_reg(&bus, &dev, &reg))
	return;
      
      printf(" => ");
      
      Io::out32(0x80000000 | (bus<<16) | (dev<<8) | (reg & ~3),PCI_CONFIG_ADDR);
      answer = Io::in32(PCI_CONFIG_DATA);
      printf("%08x\n", answer);
      break;
    case '1':
    case '2':
    case '4':
      printf("%c-byte port ", porttype);
      if ((port = get_value(16, HEX_VALUE)) == 0xffffffff)
	return;
      printf(" => ");
      switch (porttype)
	{
	case '1':
	  if (port == Pic::MASTER_OCW)
	    printf("%02x - (shadow of master-PIC register)\n", 
		   Jdb::pic_status & 0x0ff);
	  else if (port == Pic::SLAVES_OCW)
	    printf("%02x - (shadow of slave-PIC register)\n", 
		   Jdb::pic_status >> 8);
	  else
	    {
	      // CRTC index register may be overwritten by blink_cursor()
	      if ((vga_crtc_idx != 0) && (port == vga_crtc_idx+1))
		Io::out8( vga_crtc_idx_val, vga_crtc_idx);
	      if ((vga_crtc_idx != 0) && (port == vga_crtc_idx))
		answer = vga_crtc_idx_val;
	      else
		answer = Io::in8(port);
	      printf("%02x\n", answer);
	    }
	  break;
	case '2':
	  answer = Io::in16(port);
	  printf("%04x\n", answer);
	  break;
	case '4': 
	  answer = Io::in32(port);
	  printf("%08x\n", answer);
	  break;
	}
      break;
    default:
      printf("%c - unknown port type (must be 1,2,4 or p)\n", porttype);
      return;
  }
  return;
}


static 
void
Jdb::do_io_out(void)
{
  unsigned port, value, type;
  unsigned bus, dev, reg;

  putstr("ut ");
  
  int porttype=getchar();
  switch (porttype)
    {
    case 'p':
      if (!get_pci_bus_dev_reg(&bus, &dev, &reg))
	return;
      
      putstr(" : ");
  
      if (!x_get_32(&value, 0))
	return;
      
      Io::out32( 0x80000000 | (bus<<16) | (dev<<8) | (reg & ~3), PCI_CONFIG_ADDR);
      Io::out32( value, PCI_CONFIG_DATA);
      break;
      
    case '1':
    case '2':
    case '4':
      printf("%c-byte port ", porttype);
      if ((port = get_value(16, HEX_VALUE)) == 0xffffffff)
	return;
      
      putstr(" : ");

      if (!x_get_32(&value, 0))
	return;
      
      // write byte/s to port
      switch (porttype) 
	{
	case '1':
	  if (port == Pic::MASTER_OCW)
	    {
	      Jdb::pic_status = (Jdb::pic_status & 0x0ff00) | value;
	      printf(" - master-PIC mask register will be set on <g>\n");
	    }
	  else if (port == Pic::SLAVES_OCW)
	    {
	      Jdb::pic_status = (Jdb::pic_status & 0x0ff) | (value<<8);
	      printf(" - slave-PIC mask register will be set on <g>\n");
	    }
	  else 
	    {
#if 0
	      if (port == c_port)
		{
		  // CRTC register may be overwritten by blink_cursor()
		  vga_crtc_idx     = port;
		  vga_crtc_idx_val = value;
		  break;
		}
#endif
	      if ((vga_crtc_idx != 0) && (port == vga_crtc_idx+1))
		{
		  Io::out8( vga_crtc_idx_val, vga_crtc_idx);
		}
	      Io::out8( value, port );
	    }
	  break;
	case '2':
	  Io::out16( value, port );
	  break;
	case '4': 
	  Io::out32( value, port );
	  break;
	}
      break;
    case 'a':
      printf("ack IRQ 0x");
      if ((value = get_value(4, HEX_VALUE)) == 0xffffffff)
	return;

      if (value < 8)
	Io::out8( 0x60 + value, Pic::MASTER_ICW);
      else 
	{
	  Io::out8( 0x60 + (value & 7), Pic::SLAVES_ICW);
	  Io::out8( 0x60 + 2, Pic::MASTER_ICW);
	}

      break;
    case 'm':
      printf("(Un)Mask? ");
      type = getchar();
      putstr("\b\b\b\b\b\b\b\b\b\b");

      switch (type) 
	{
	case 'u':
	  printf("un");
	  /* fall through */
	case 'm':
	  printf("mask IRQ 0x");
	  if ((value = get_value(4, HEX_VALUE)) == 0xffffffff)
	    return;

	  switch (type) {
	    case 'm':
	      Jdb::pic_status |= (1 << value);
	      printf(" - PIC mask will be modified on \"g\"\n");
	      break;
	    case 'u':
	      Jdb::pic_status  &= ~(Unsigned16)(1 << value);
	      printf(" - PIC mask will be modified on \"g\"\n");
	      break;
	  }
	  break;
	default:
	  printf("unknown mode: type m(ask) or u(nmask)\n");
	  return;
	}
      break;
      
    default:
      printf("%c - unknown mode (must be 1,2,4, a, m or p)\n", porttype);
      return;
    }

  putchar('\n');
  return;
}

static 
bool
Jdb::dump_address_space(trap_state *ts, char dump_type, 
			Address address, unsigned task, int level=0)
{
  unsigned actx = 0;		// current cursor x
  unsigned acty = 0;		// current cursor y
  unsigned absy = 0;		// y page base
  unsigned max_absy;		// number of all lines
  unsigned elc;			// entry_lenght in chars

#define ELB	4		// entry_lenght in bytes
#define EPL	8		// entries per line
#define BOL	9		// begin of line
#define LINES	24		// number of lines to dump

#define LINE_OFFS (EPL*ELB)
#define LINE_MASK (LINE_OFFS-1)
#define LINE_PAGE (Config::PAGE_SIZE / LINE_OFFS)
 
  Address entry_addr = 0;	// whats in statusbar between <>
  Address entry_addr_k = 0;	// counterpart of entry_addr in kernel addr
  Address entry_offs;		// space between entries

  static unsigned ptab_offs;
  unsigned entry = 0;
  pd_entry_t *pdir = 0;
  
  if (level==0)
    fancy_clear_screen();

 dump_mode:
  
  if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
    { 
      absy = (address & ~Config::PAGE_MASK) / LINE_OFFS;
      address &= Config::PAGE_MASK;
      elc = 8;
      max_absy = LINE_PAGE - 24;
      entry_offs = (dump_type==PAGE_DIR) 
			? Config::PAGE_SIZE * 1024 
			: Config::PAGE_SIZE;
      pdir = 0;
      unsigned pa = Kmem::virt_to_phys((void*)address);
      if (pa != PAGE_INVALID)
	pdir = pdir_find_pde(pa, 0);
    }
  else // D/B/C-MODE
    {
      absy = address / LINE_OFFS;
      actx = (address & LINE_MASK) / ELB;
      address &= ~LINE_MASK;
      elc = (dump_type==C_MODE) ? 4 : 8;
      max_absy = (0 - 24*LINE_OFFS) / LINE_OFFS;
      entry_offs = ELB;
    }
  
  if (absy > max_absy)
    {
      unsigned diff = absy-max_absy;
      absy -= diff;
      acty += diff;
    }
  
 screen:
  
  // dump current screen
  cursor(0,0);

  do_dump_memory(LINES, dump_type, absy, address, 
		 task, elc, &entry);

  for (;;)
    {
      entry_addr = ((absy+acty)*EPL + actx)*entry_offs;
      
      if (dump_type==PAGE_TAB) 
	entry_addr += ptab_offs;
      
      if (dump_type==D_MODE || dump_type==B_MODE)
	entry_addr_k = k_lookup(entry_addr, task);
      
      if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
	{
	  printf_statline("p<%08x> task %-3x %53s",
	      entry_addr, task,
      	      "KEYS: Arrows PgUp PgDn Home End Space CR");
	}
      else  // D/B/C-MODE
	{
	  address = absy*LINE_OFFS + actx*entry_offs;
	  printf_statline("%c<%08x> task %-3x %53s",
      	      dump_type, entry_addr, task,
	      (dump_type==D_MODE)
	      ? "KEYS: Arrows PgUp PgDn Home End <D>ump <e>dit CR"
	      : "KEYS: Arrows PgUp PgDn Home End <D>ump CR");
	}
      
      // mark next value
      cursor(acty, (elc+1)*actx+BOL);
      switch(int c=getchar())
	{
	case KEY_CURSOR_UP:
	  if (acty > 0)
	    acty--;
	  else if (absy > 0)
    	    {
	      absy--; 
	      goto screen;
	    }
	  break;
	case KEY_CURSOR_DOWN:
	  if (acty < 23) 
	    acty++;
	  else if (absy < max_absy)
    	    {
	      absy++;
	      goto screen;
	    }
	  break;
	case KEY_CURSOR_RIGHT:
	  if (actx < 7)
	    actx ++;
	  else if (acty < 23)
	    { 
      	      acty++; 
	      actx = 0;
	    } 
	  else if (absy < max_absy)
    	    { 
	      absy++;
	      actx = 0;
	      goto screen;
	    }
	  break;
     	case KEY_CURSOR_LEFT:
	  if (actx > 0)
	    actx--;
	  else if (acty > 0)
	    {
      	      acty--;
	      actx = 7;
	    }
	  else if (absy > 0)
    	    { 
	      absy--;
	      actx = 7;
	      goto screen;
	    }
	  break;
	case KEY_PAGE_UP:
	  if (absy == 0)
	    {
	      actx = 0;
	      acty = 0;
	      break;
	    } 
	  if (absy > 23)
    	    absy -= 23;
	  else
	    absy=0;
	  goto screen;
	case KEY_PAGE_DOWN:
	  if (absy == max_absy)
	    {
	      actx = 7;
	      acty = 23;
	      break;
	    }
	  if (absy < max_absy - 23)
    	    absy += 23;
	  else 
	    absy = max_absy;
	  goto screen;
	case KEY_CURSOR_HOME: // return to previous or go home
	  if (level == 0)
	    {
	      if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
		absy = 0;
	      else // D/B/C-MODE
		if (absy > 0)
		  absy = (absy + acty - 1 + ((actx+6)/7)) & ~(LINE_PAGE-1);
	      actx = 0;
	      acty = 0;
	      goto screen;
	    }
	  return true;
	case KEY_CURSOR_END:
	  if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
	    absy = max_absy;
	  else
	    if (absy < max_absy)
      	      absy = ((absy + acty + actx/7) & ~(LINE_PAGE-1)) + LINE_PAGE - 24;
	  actx = 7; 
      	  acty = 23;
    	  goto screen;
	case 'D': // dump user specified range of memory gzipped + uuencoded
		  // to (serial) console
	  cursor(LAST_LINE, 26);
	  clear_to_eol();

	  vm_offset_t low_addr, high_addr;
	  if (x_get_32_interval(&low_addr, &high_addr))
	    {
	      low_addr  &= ~(LINE_OFFS-1);
	      high_addr  =  (high_addr + (2*LINE_OFFS-1)) & ~(LINE_OFFS-1);
	      
	      if (low_addr <= high_addr)
		{
		  unsigned lines = (high_addr - low_addr - 1) / LINE_OFFS;
		  unsigned tmp_entry = entry;
		  
		  if (lines < 1)
	      	    lines = 1;

		  dump_gzip = true;
		  printf("\n=== start of dump ===\n");
		  do_dump_memory(lines, dump_type, low_addr/LINE_OFFS, low_addr,
				 task, elc, &tmp_entry);
		  printf("=== end of dump ===\n");
		  dump_gzip = false;
		}
	    }
	  
	  goto screen;
	case ' ': // change viewing mode
	  switch (dump_type)
	    {
	    case D_MODE:
	      dump_type=B_MODE;
	      break;
	    case B_MODE:
	      dump_type=C_MODE;
	      break;
	    case C_MODE:
	      dump_type=PAGE_DIR;
	      break;
	    case PAGE_DIR:
	    case PAGE_TAB:
	      // save absy and actx for D-MODE (address is page aligned)
	      address += absy*LINE_OFFS + actx*ELB;
	      dump_type=D_MODE;
	      break;
	    }
	  goto dump_mode;
	case KEY_TAB:
	  show_phys_mem = !show_phys_mem;
	  goto screen;
	case KEY_RETURN: // goto address under cursor
	  if (level > 6) // limit # of recursive calls
	    break;
	  switch (dump_type)
	    {
	    case C_MODE:
	    case B_MODE:
	      break;
	    case PAGE_DIR:
	    case PAGE_TAB:
	      if (!pdir)
		break;
	      entry = pdir[(absy+acty)*EPL + actx];
	      if (!(entry & INTEL_PDE_VALID))
		break;
	      if (dump_type==PAGE_DIR && !(entry & INTEL_PDE_SUPERPAGE))
		{
	      	  ptab_offs = entry_addr;
      		  if (!dump_address_space(ts, PAGE_TAB,
				  (Address)Kmem::phys_to_virt(pde_to_pa(entry)),
				  task, level+1))
      		    return false;
		}
	      else
		{
		  if (!dump_address_space(ts, D_MODE, entry_addr, 
					  task, level+1))
		    return false;
		}
	      break;
    	    case D_MODE:
    	      if (  (entry_addr_k != PAGE_INVALID)
		  &&(Kmem::virt_to_phys((void*)entry_addr_k) != PAGE_INVALID))
		{
		  if (!dump_address_space(ts, D_MODE, *(Address*)entry_addr_k,
					  task, level+1))
		    return false;
		}
	      break;
    	    }
	  goto screen;

	case 'u':
	  if ((disasm_bytes != 0) && (dump_type == D_MODE))
	    {
	      if (  (entry_addr_k != PAGE_INVALID)
		  &&(Kmem::virt_to_phys((void*)entry_addr_k) != PAGE_INVALID))
		{
		  Address dis_addr = *(Address*)entry_addr_k;
		  int c1;

		  printf_statline("u[address=%08x task=%x] ", dis_addr, task);
		  c1 = getchar();
		  if (c1 != KEY_RETURN)
		    {
		      unsigned new_addr, new_task;

		      printf_statline(0);
		      putchar('u');
		      if (!get_task_address(&new_addr, &new_task, 
					    ts->eip, ts, c1))
			return false;

		      disasm_address_space(new_addr, new_task);
		      return false;
		    }

		  if (!disasm_address_space(dis_addr, task, level+1))
		    return false;

		  goto screen;
		}
	    }
	  break;

	case 'e': // poke memory
	  if (   (dump_type == D_MODE) 
	      && (entry_addr_k != PAGE_INVALID))
	    {
	      unsigned page, offs;
	      bool superpage;

	      page = page_lookup(entry_addr, task, &superpage, &offs);
	      if (page >= -Kmem::mem_phys)
		{
		  if (!show_phys_mem)
		    // don't show -- don't modity adapter pages
		    break;

		  if (superpage)
    		    page += entry_addr & ~Config::SUPERPAGE_MASK;
		  else
		    page += entry_addr & ~Config::PAGE_MASK;

		  // This is an adapter page. If we want to view it's contents
		  // we have to make sure that this page is mapped into our
		  // address space. The Jdb_adapter_page is reserved for that
		  // purpose.
		  page = establish_phys_mapping(page, &offs);
		}
	      else
		page = (unsigned)Kmem::phys_to_virt(page);

	      cursor(acty, (elc+1)*actx+BOL);
	      printf("        ");
	      printf_statline("edit <%08x> = %08x",
			      entry_addr, *(unsigned*)(page+offs));
	      cursor(acty, (elc+1)*actx+BOL);
	      unsigned value;
	      if (!x_get_32(&value, 0))
		{
		  cursor(acty, (elc+1)*actx+BOL);
		  printf("%08x", *(unsigned*)(page+offs));
		  break;
		}

	      *(unsigned*)(page+offs) = value;
	      goto screen;
	    }
	  break;
	case KEY_ESC:
	  abort_command();
	  return false;
	default:
	  if (is_toplevel_cmd(c)) 
	    return false;
	}
    }
}

static void
Jdb::do_dump_memory(unsigned lines, const char dump_type, 
		    const unsigned absy, unsigned address,
		    const unsigned task, const unsigned elc,
		    unsigned * const entry)
{
  bool superpage = false;
  char buf[16];
  pd_entry_t *pdir = 0;

  if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
    { 
      address &= Config::PAGE_MASK;
      unsigned pa = Kmem::virt_to_phys((void*)address);
      if (pa != PAGE_INVALID)
	pdir = pdir_find_pde(pa, 0);
    }

  if (dump_gzip)
    {
      if (gz_init_done)
	gz_open("dump.gz");
      else
	dump_gzip = false;
    }

  for (unsigned i=0, act_page=0, act_offs=0; i<lines; i++, act_offs+=LINE_OFFS)
    {
      unsigned act_line = (i + absy)*LINE_OFFS;

      if (!dump_gzip)
	getchar_chance();

      if (dump_type==PAGE_DIR || dump_type==PAGE_TAB)
	act_line += address;
     
      sprintf(buf, "%08x", act_line);
      if (dump_gzip)
	gz_write(buf, 8);
      else
	printf("%s", buf);
      
      if (  (dump_type==D_MODE || dump_type==B_MODE || dump_type==C_MODE)
	  &&(  (i==0)
	     ||( superpage && ((act_line & ~Config::SUPERPAGE_MASK) == 0))
	     ||(!superpage && ((act_line & ~Config::PAGE_MASK)      == 0))))
	{
	  act_page = page_lookup(act_line, task, &superpage, &act_offs);
	  if (   (act_page != PAGE_INVALID)	/* valid mapping */
	      && (act_page >= -Kmem::mem_phys)	/* non-memory page */
	      && show_phys_mem)
	    {
	      // Thisis an adapter page. If we want to view it's contents
	      // we have to make sure that this page is mapped into our
	      // address space. The Jdb_adapter_page is reserved for that
	      // purpose.
	      if (superpage)
		{
		  act_page += act_line & ~Config::SUPERPAGE_MASK;
		  superpage = false;
		}
	      else
		{
		  act_page += act_line & ~Config::PAGE_MASK;
		}

	      // establish a mapping of physical page to Jdb_adapter_page
	      act_page = establish_phys_mapping(act_page, &act_offs);

	      // correct kmem::phys_to_virt some code lines later
	      act_page -= Kmem::mem_phys;
	    }
	}
     
      Apic::ignore_invalid_reg_access = true;

      // dump one line
      for (unsigned x=0; x < EPL; x++)
	{
	  buf[0] = (x == 0) ? ':' : ' ';
	  
	  if (dump_type==D_MODE || dump_type==B_MODE || dump_type==C_MODE)
	    {
	      if (act_page == PAGE_INVALID)
		{
		  for (unsigned u=0; u<elc; u++)
		    buf[u+1] = '.';
		}
	      else
		{
		  unsigned pa = act_page + act_offs + x*ELB;
		  unsigned va = (unsigned)Kmem::phys_to_virt(pa);
		  
		  // check if address references a physical memory page
		  if (pa >= -Kmem::mem_phys && !show_phys_mem)
		      // No: Address points to adapter page. We do not want
		      // to dump adapter pages because reading of them might
		      // be dangerous.
		    {
		      for (unsigned u=0; u<elc; u++)
			buf[u+1] = '-';
		    }
		  else if (dump_type==D_MODE)
		    {
		      unsigned c = *(unsigned *)va;
		      if (c == 0)
			strcpy(buf+1, "       0");
		      else if (c == 0xffffffff)
			strcpy(buf+1, "      -1");
		      else
	      		sprintf(buf+1, "%08x", c);
		    } 
		  else if (dump_type==B_MODE)
		    {
		      for (char u=0; u<ELB; u++)
	      		sprintf(buf+1+2*u, "%02x", *(unsigned char*)(va+u));
		    }
		  else // C_MODE
		    {
		      for (char u=0; u<ELB; u++)
			{
			  unsigned char c = *(unsigned char*)(va+u);
			  buf[u+1] = (c>=32 && c<=126) ? c : '.';
			}
		    }
		}
	    }
	  else // PAGETABLES
	    {
	      if (!pdir)
		{
		  for (unsigned u=0; u<elc; u++)
		    buf[u+1] = '.';
		}
	      else
		{
		  *entry = pdir[(i + absy)*EPL + x];
		  if (!(*entry & INTEL_PDE_VALID))
		    strcpy(buf+1, "    -   ");
		  else
		    {
		      unsigned pa = pde_to_pa(*entry);
	    	      if (pa > 0x40000000)
    			{
			  if (*entry & INTEL_PDE_SUPERPAGE)
			    sprintf(buf+1, "%03X/4--", 
				pa >> (Config::SUPERPAGE_SHIFT-2));
			  else
			    sprintf(buf+1, "%05X--", 
				pa >> Config::PAGE_SHIFT);
			}
		      else
			{
			  pa &= HI_MASK;
			  if (*entry & INTEL_PDE_SUPERPAGE)
			    sprintf(buf+1, " %02x/4--", 
				pa >> Config::SUPERPAGE_SHIFT);
			  else
			    sprintf(buf+1, " %04x--", 
				pa >> Config::PAGE_SHIFT);
			}
		      
		      buf[8] = ((*entry & INTEL_PDE_USER)
			     ? (*entry & INTEL_PDE_WRITE) ? 'w' : 'r'
			     : (*entry & INTEL_PDE_WRITE) ? 'W' : 'R');
		    }
		}
	    }
	  
	  buf[elc+1] = 0;
	  if (dump_gzip)
	    gz_write(buf, strlen(buf));
	  else
	    printf("%s", buf);
	}
      
      Apic::ignore_invalid_reg_access = false;

      // end of line
      if (dump_type==C_MODE && !dump_gzip)
      	clear_to_eol();
      
      if (dump_gzip)
	gz_write("\n", 1);
      else
	putchar('\n');
    }
  
  if (dump_gzip)
    gz_close();
}

// Searches for a value in memory, starting at physical address if task==0.
// This means, that memory of all tasks is searched. Returns address of
// first apperance if found.
static
bool
Jdb::search_dword(unsigned string, unsigned *address)
{
  // Search in physical memory
      
  // Funny things happen when reading from something like io-ports and 
  // display buffers so I skiped their memory locations (reserved windows) 
  // for searching.
  unsigned a, a_top;
  
  // search in <0> ... <start of Fiasco kernel>
  a     = (*address) ? *address : Kmem::mem_phys;
  a_top = (unsigned)Kmem::phys_to_virt(Kmem::info()->reserved0.low-4);
  if (*address < a_top)
    {
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }

  // search in <end of Fiasco kernel + 1> ... <0x000A0000>
  a     = (unsigned)Kmem::phys_to_virt(Kmem::info()->reserved0.high+1);
  a_top = (unsigned)Kmem::phys_to_virt(Kmem::info()->semi_reserved.low-4);
  if (*address < a_top)
    {
      if (a < *address)
	a = *address;
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }
  
  // search in <0x00100000> ... <end of main memory>
  a     = (unsigned)Kmem::phys_to_virt(Kmem::info()->semi_reserved.high+1);
  a_top = Kmem::mem_phys+Kmem::info()->main_memory.high-4;
  if (*address < a_top)
    {
      if (*address > a)
	a = *address;
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }
  
  return false;
}

static 
int
Jdb::show_mapping()
{
  Mapping_tree *t;
  Mapping *m;
  unsigned i, page, address;
  
  int c=getchar();
  switch (c)
    {
    case 'p':
      printf(" page:");
      if ((page = get_value(20, HEX_VALUE)) == 0xffffffff)
	return false;

      if (page > Kmem::info()->main_memory.high / Config::PAGE_SIZE)
	{
	  printf(" non extisting phys. page\n");
	  return false;
	}
      break;
      
    case 'a':
      printf(" address:");
	  
      if (!x_get_32(&address, 0)) 
	return false;
	  
      t = (Mapping_tree*)address;
      page = t->mappings()->data()->address;
      
      if (page > Kmem::info()->main_memory.high / Config::PAGE_SIZE)
	{
	  printf(" non extisting phys. page\n");
	  return false;
	}
      break;
      
    case KEY_ESC:
    default:
      abort_command();
      return false;
    }
 
  fancy_clear_screen();
  
  for (;;)
    {
      t = mapdb_instance()->physframe[page].tree.get();
      
      home();
      printf("\033[K\n"
	     " mapping tree for page %08x - header at %08x\033[K\n"
	     "\033[K\n",
	     page << 12,(unsigned)t);
      printf(" header info: "
	     "entries used: %u  free: %u  total: %u  lock=%u\033[K\n"
	     "\033[K\n",
	     t->_count,t->_empty_count, t->number_of_entries(),
	     mapdb_instance()->physframe[page].lock.test());
  
      if (t->_count + t->_empty_count > t->number_of_entries())
	{
	  printf("\033[K\n"
		 "\033[K\n"
		 "  seems to be a wrong tree ! ...exiting");
	  // clear rest of page
	  for (unsigned i=6; i<25; i++)
	    printf("\033[K\n");
	  return false;
	}

      m = t->mappings();

      for (i=0; i < t->number_of_entries(); i++, m++)
	{
	  unsigned indent = m->data()->depth;
	  if (indent > 10)
	    indent = 0;
      
	  printf("%*u: %x  va=%08x  task=%03x  size=%u  depth=",
		 indent+3, i+1,
		 (unsigned)(m->data()), (unsigned)(m->data()->address << 12),
		 m->data()->space, m->data()->size);
      
	  if (m->data()->depth == Depth_root)
	    printf("root");
	  else if (m->data()->depth == Depth_empty)
	    printf("empty");
	  else if (m->data()->depth == Depth_end)
	    printf("end");
	  else
	    printf("%u",m->data()->depth);
	  
	  printf("\033[K\n");

	  if (i % 18 == 17)
	    {
	      printf(" any key for next side or <ESC>");
	      cursor(24, 32);
	      c=getchar();
	      printf("\r\033[K");
	      if (c == KEY_ESC) 
		return false;
	      cursor(5, 0);
	    }
	}

      for (; i<18; i++)
	printf("\033[K\n");

      printf_statline("%74s", "KEYS: <n>ext <p>revious page");
      
      for (bool redraw=false; !redraw; )
	{
	  cursor(LAST_LINE, LOGO+1);
	  switch (c=getchar()) 
	    {
	    case 'n':
	    case KEY_CURSOR_DOWN:
	      if (++page > Kmem::info()->main_memory.high / Config::PAGE_SIZE)
      		page =  0;
	      redraw = true;
	      break;
	    case 'p':
	    case KEY_CURSOR_UP:
	      if (--page > Kmem::info()->main_memory.high / Config::PAGE_SIZE)
      		page =  Kmem::info()->main_memory.high / Config::PAGE_SIZE;
	      redraw = true;
	      break;
	    case KEY_ESC:
	      abort_command();
	      return false;
	    default:
	      if (is_toplevel_cmd(c)) 
		return false;
	    }
	}
    }
  return true;
}

static
int
Jdb::write_tsc_s(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  int len = 0;

  if (tsc < 0)
    uns = -uns;

  if (uns >= 1000000000000ULL)
    {
      strncpy(buf, ">99        s ", maxlen);
      len = 13;
    }
  else
    {
      if (sign)
	{
	  *buf++ = (tsc < 0) ? '-' : (tsc == 0) ? ' ' : '+';
	  maxlen--;
	  len = 1;
	}
      for (; maxlen>14; maxlen--, len++)
	*buf++ = ' ';
      if (uns >= 1000000000)
	{
	  Mword _s  = uns / 1000000000ULL;
	  Mword _us = (uns - 1000000000ULL * (Unsigned64)_s) / 1000ULL;
	  len += snprintf(buf, maxlen, "%3u.%06u s ", _s, _us);
	}
      else if (uns == 0)
	{
	  strncpy(buf, "  0          ", maxlen);
	  len += 13;
	}
      else
	{
	  Mword _ms = uns / 1000000UL;
	  Mword _ns = (uns - 1000000UL * (Unsigned64)_ms);
	  len += snprintf(buf, maxlen, "%3u.%06u ms", _ms, _ns);
	}
    }

  return len;
}

static
int
Jdb::write_tsc(Signed64 tsc, char *buf, int maxlen, bool terminal, bool sign)
{
  Unsigned64 ns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  if (tsc < 0)
    ns = -ns;
  return write_ns(ns, buf, maxlen, terminal, sign);
}

static
int
Jdb::write_ns(Signed64 ns, char *buf, int maxlen, bool terminal, bool sign)
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
	  len += snprintf(buf, maxlen, "%3u.%03u s ", _s, _ms);
	}
      else if (uns >= 1000000)
	{
	  Mword _ms = uns / 1000000UL;
	  Mword _us = (uns - 1000000UL * (Unsigned64)_ms) / 1000UL;
	  len += snprintf(buf, maxlen, "%3u.%03u ms", _ms, _us);
	}
      else if (uns == 0)
	{
	  strncpy(buf, "  0       ", maxlen);
	  len += 10;
	}
      else
	{
	  Mword _us = uns / 1000UL;
	  Mword _ns = (uns - 1000UL * (Unsigned64)_us);
	  len += snprintf(buf, maxlen, "%3u.%03u %c ",
				      _us, _ns, terminal ? 'µ' : '\346');
	}
    }

  return len;
}

static
int
Jdb::write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign)
{
  // display 40 bits
  Unsigned64 xu = (x < 0) ? -x : x;
  return snprintf(buf, maxlen, "%s%02x%08x",
		  sign ? (x < 0) ? "-" : (x == 0) ? " " : "+" : "",
		  (Mword)((xu >> 32) & 0xff), (Mword)xu);
}

static
int
Jdb::write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign)
{
  int len;
  Unsigned64 xu = (x < 0) ? -x : x;
  
  // display decimal
  if (xu > 0xFFFFFFFFULL)
    {
      strncpy(buf, " >>       ", maxlen);
      len = 10;
    }
  else
    len = snprintf(buf, maxlen, "%s%10u",
		   sign ? x < 0 ? "-" : (x == 0) ? " " : "+"  : "",
		   (Mword)xu);

  return len;
}

static
void
Jdb::show_tracebuffer_events(Mword start, Mword ref, Mword count, 
			     char mode, char time_mode)
{
  if (dump_gzip)
    {
      if (gz_init_done)
	gz_open("trace.gz");
      else
	dump_gzip = false;
    }

  Unsigned64 ref_tsc, ref_pmc;
  Mword dummy;

  Jdb_tbuf::event(ref, &dummy, &ref_tsc, &ref_pmc);

  for (Mword n=start; n<start+count; n++)
    {
      static char buffer[160];
      Mword number;
      Signed64 delta;
      Unsigned64 utsc, upmc;

      getchar_chance();

      if (!Jdb_tbuf::event(n, &number, &utsc, &upmc))
	break;

      if (dump_gzip)
	{
	  sprintf(buffer, "%10u.  ", number);
	  char *p = buffer + 13;
	  int maxlen = sizeof(buffer)-13, len;
	  
	  Jdb_tbuf_output::print_entry(n, p, 72);
	  p += 71;
	  *p++ = ' ';	// print_entry terminates with '\0'
	  maxlen -= 72;
	  
	  if (!Jdb_tbuf::diff_tsc(n, &delta))
	    delta = 0;
	  
	  len = write_ll_dec(delta, p, 12, false);
	  p += len;
	  *p++ = ' ';
	  *p++ = '(';
	  maxlen -= len+2;
	  len = write_tsc_s(delta, p, 15, false);
	  p += len;
	  *p++ = ')';
	  *p++ = ' ';
	  *p++ = ' ';
	  maxlen -= len+3;
	  len = snprintf(p, maxlen, "%16lld", utsc);
	  p += len;
	  *p++ = ' ';
	  *p++ = '(';
	  maxlen -= len+2;
	  len = write_tsc_s(utsc, p, 15, false);
	  p += len;
	  *p++ = ')';
	  *p++ = '\n';
	  maxlen -= len+2;

	  gz_write(buffer, sizeof(buffer)-maxlen);
	}
      else
	{
	  int maxlen = 12;
	  char *p = buffer + 80-maxlen;
	  int len = 0;

	  if (n == ref)
	    putstr("\033[1;32m");

	  Jdb_tbuf_output::print_entry(n, buffer, 80-maxlen);
	  p[-1] = ' ';	// print_entry terminates with '\0'
      
	  switch (mode)
	    {
	    case INDEX_MODE:
	      len = snprintf(p, maxlen, "%11u", number);
	      break;
	    case DELTA_TSC_MODE:
	      if (!Jdb_tbuf::diff_tsc(n, &delta))
		delta = 0;
	      switch (time_mode)
		{
		case 0: len = write_ll_hex(delta, p, maxlen, false); break;
		case 1: len = write_tsc(delta, p, maxlen, false, false); break;
		case 2: len = write_ll_dec(delta, p, maxlen, false); break;
		}
	      break;
	    case REF_TSC_MODE:
	      delta = (n == ref) ? 0 : utsc - ref_tsc;
	      switch (time_mode)
		{
		case 0: len = write_ll_hex(delta, p, maxlen, true); break;
		case 1: len = write_tsc(delta, p, maxlen, false, true); break;
		case 2: len = write_ll_dec(delta, p, maxlen, true); break;
		}
	      break;
	    case START_TSC_MODE:
	      delta = utsc;
	      switch (time_mode)
		{
		case 0: len = write_ll_hex(delta, p, maxlen, true); break;
		case 1: len = write_tsc(delta, p, maxlen, false, false); break;
		case 2: len = write_ll_dec(delta, p, maxlen, true); break;
		}
	      break;
	    case DELTA_PMC_MODE:
	      if (!Jdb_tbuf::diff_pmc(n, &delta))
		delta = 0;
	      len = write_ll_dec(delta, p, maxlen, false);
	      break;
	    case REF_PMC_MODE:
	      delta = (n == ref) ? 0 : upmc - ref_pmc;
	      len = write_ll_dec(delta, p, maxlen, true);
	      break;
	    }
	  
	  for (int i=len; i<maxlen; i++)
	    p[i] = ' ';
	  p[maxlen] = '\0';
	  puts(buffer);

	  if (n == ref)
	    putstr("\033[m");
	}
    }

  if (dump_gzip)
    gz_close();
}

// search in tracebuffer
static
Mword
Jdb::search_tracebuffer(Mword start, const char *str, Unsigned8 direction)
{
  int first = 1;

  if (!Jdb_tbuf::entries())
    return (Mword)-1;

  if (direction==1)
    start--;
  else
    start++;

  for (Mword n=start; ; (direction==1) ? n-- : n++)
    {
      static char buffer[120];

      if (first)
	first = 0;
      else if (n == start)
	break;

      if (!Jdb_tbuf::event_valid(n))
	n = (direction==1) ? Jdb_tbuf::entries()-1 : 0;

      Jdb_tbuf_output::print_entry(n, buffer, sizeof(buffer));
      if (strstr(buffer, str))
	return n;
    }

  return (Mword)-1;
}

static
int
Jdb::show_tracebuffer()
{
  static char mode = INDEX_MODE;
  static char time_mode = 1;
  static char search_str[40];
  static Unsigned8 direction = 0;
  const char * const status = 
	 "KEYS: / ? n <r>ef <c>lear <P>erf Arrows PgUp PgDn Home End Space CR";

  Mword fst_nr = 0;		// nr of first event seen on top of screen

  Mword absy  = show_tb_absy;	// idx or first event seen on top of screen
  Mword refy  = show_tb_refy;	// idx of reference event
  Mword addy  = 0;		// cursor position starting from top of screen
  Mword lines = SHOW_TRACEBUFFER_LINES;
  Mword entries = Jdb_tbuf::entries();

  if (Config::tbuf_entries < lines)
    lines = Config::tbuf_entries;

  if (entries < lines)
    lines = entries;

  if (refy > entries-1)
    refy = entries-1;

  Mword max_absy = entries > lines ? entries - lines : 0;

  if (entries)
    {
      Unsigned64 dummyll;

      Jdb_tbuf::event(absy, &fst_nr, &dummyll, &dummyll);
      addy = fst_nr - show_tb_nr;
      if ((show_tb_nr == (Mword)-1) || (addy > SHOW_TRACEBUFFER_LINES))
	addy = 0;
    }

  fancy_clear_screen();

  for (;;)
    {
      static const char * const mode_str[] = 
	{ "index", "tsc diff", "tsc rel", "tsc start", "pmc diff", "pmc rel" };

      Mword perf_event;
      Unsigned64 dummyll;
      const char *perf_type, *perf_mode;
      int have_perf, have_tsc, perf_user, perf_kernel;

      if (entries)
	Jdb_tbuf::event(absy, &fst_nr, &dummyll, &dummyll);

      have_tsc  = Jdb_perf_cnt::have_tsc();

      have_perf = Jdb_perf_cnt::perf_mode(&perf_type, &perf_mode,
					  &perf_event, &perf_user,
					  &perf_kernel);

      home();
      printf("   [TRACEBUFFER - max:%u used:%u perf:%s,%04x(%s)]\033[K",
    	  Config::tbuf_entries, entries, perf_type, perf_event, perf_mode);
      cursor(0, 69);
      printf("%s\n", mode_str[mode]);

      for (Mword i=1; i<SHOW_TRACEBUFFER_START; i++)
	printf("\033[K\n");
      
      show_tracebuffer_events(absy, refy, lines, mode, time_mode);

      for (Mword i=SHOW_TRACEBUFFER_START+lines; i<24; i++)
	printf("\033[K\n");

      printf_statline("T%73s", status);
      
      for (bool redraw=false; !redraw;)
	{
	  Mword pair_event, pair_y;
	  Smword c;
	  Unsigned8 type;
	  Unsigned8 d = 0; // default search direction is forward
	  
	  // search for paired ipc event
	  pair_event = Jdb_tbuf::ipc_pair_event(absy+addy, &type);
	  if (pair_event == (Mword)-1)
	    // search for paired pagefault event
	    pair_event = Jdb_tbuf::pf_pair_event(absy+addy, &type);
	  if (pair_event > fst_nr || pair_event <= fst_nr-lines)
	    pair_event = (Mword)-1;
	  pair_y = fst_nr-pair_event;
	  if (pair_event != (Mword)-1)
	    {
    	      cursor(pair_y+SHOW_TRACEBUFFER_START, 0);
	      putstr("\033[1;33m");
	      switch (type)
		{
		case Jdb_tbuf::RESULT: putstr("+++>"); break;
		case Jdb_tbuf::EVENT:  putstr("<+++"); break;
		}
	      putstr("\033[m");
	    }
	  cursor(addy+SHOW_TRACEBUFFER_START, 0);
	  putstr("\033[1;33m");
	  show_tracebuffer_events(absy+addy, refy, 1, mode, time_mode);
	  putstr("\033[m");
	  cursor(addy+SHOW_TRACEBUFFER_START, 0);
	  c=getchar();
	  show_tracebuffer_events(absy+addy, refy, 1, mode, time_mode);
	  if (pair_event != (Mword)-1)
	    {
	      cursor(pair_y+SHOW_TRACEBUFFER_START, 0);
	      show_tracebuffer_events(absy+pair_y, refy, 1, mode, time_mode);
	    }
	  switch (c)
	    {
	    case 'D':
    	      cursor(LAST_LINE, 16);
	      clear_to_eol();
	      printf("Count=");

	      unsigned count;
	      if ((count = get_value(28, DEC_VALUE)) != 0xffffffff)
		{
	     	  if (count == 0)
    		    count = lines;
		  dump_gzip = true;
		  printf("\n=== start of trace ===\n");
		  show_tracebuffer_events(absy, refy, count, mode, time_mode);
		  printf("=== end of trace ===\n");
		  dump_gzip = false;
		  redraw = true;
		}
	      break;
	    case 'P':
	      if (have_perf)
		{
		  unsigned event = perf_event, e;
		  int user=perf_user, kernel=perf_kernel;
		  
		  cursor(LAST_LINE, LOGO);
		  putchar(c);
		  switch(c=getchar())
		    {
		    case '+': user = kernel = 1; break;
		    case '-': user = kernel = 0; break;
		    case 'u': user = 1; kernel = 0; break;
		    case 'k': user = 0; kernel = 1; break;
		    default:
		      if ((e = get_value(16, HEX_VALUE, c)) != 0xffffffff)
			event = e;
		      break;
		    }
		  
		  Jdb_perf_cnt::init_pmc(event, user, kernel);

		  redraw = true;
		}
	      break;
	    case KEY_RETURN:
	      if (disasm_bytes != 0)
		{
		  L4_uid tid;
		  Mword eip;
		  if (   Jdb_tbuf_output::thread_eip(absy+addy, &tid, &eip)
		      && check_thread(tid))
		    {
		      if (!disasm_address_space(eip, tid.task(), 1))
			goto return_false;
		      redraw = true;
		    }
		}
	      break;
	    case KEY_CURSOR_UP:
	      if (addy == 0)
		{
		  if (absy > 0)
		    {
		      absy--;
		      redraw = true;
		    }
		  break;
		}
	      addy--;
	      break;
	    case KEY_CURSOR_DOWN:
	      if (++addy >= lines)
		{
		  addy--;
		  if (absy < max_absy)
		    {
		      absy++;
		      redraw = true;
		    }
		}
	      break;
	    case KEY_CURSOR_HOME:
	      addy = 0;
	      if (absy > 0)
		{
		  absy = 0;
		  redraw = true;
		}
	      break;
	    case KEY_CURSOR_END:
	      addy = lines-1;
	      if (absy < max_absy)
		{
		  absy = max_absy;
		  redraw = true;
		}
	      break;
	    case KEY_PAGE_UP:
	      if (absy >= lines)
		{
		  absy -= lines;
		  redraw = true;
		}
	      else
		{
		  addy = 0;
		  if (absy > 0)
		    {
		      absy = 0;
		      redraw = true;
		    }
		}
	      break;
	    case KEY_PAGE_DOWN:
	      if (absy+lines-1 < max_absy)
		{
		  absy += lines; 
		  redraw = true;
		}
	      else 
		{
		  addy = lines-1;
		  if (absy < max_absy)
		    {
		      absy = max_absy;
		      redraw = true;
		    }
		}
	      break;
	    case KEY_CURSOR_LEFT:
	      mode--;
	      if (mode < 0)
		{
		  if (!have_tsc)
		    mode = INDEX_MODE;
		  else if (!have_perf)
		    mode = START_TSC_MODE;
		  else
		    mode = REF_PMC_MODE;
		}
	      redraw = true;
	      break;
	    case KEY_CURSOR_RIGHT:
	      mode++;
	      if (   (!have_tsc  && (mode>INDEX_MODE))
		  || (!have_perf && (mode>START_TSC_MODE))
		  || (              (mode>REF_PMC_MODE)))
		mode = INDEX_MODE;
	      redraw = true;
	      break;
	    case ' ': // mode switch
	      if (mode != INDEX_MODE)
		{
		  time_mode = (time_mode+1) % 3;
		  redraw = true;
		}
	      break;
	    case 'c':
	      Jdb_tbuf::clear_tbuf();
	      abort_command();
	      show_tb_absy = 0;
	      show_tb_refy = 0;
    	      show_tb_nr   = (Mword)-1;
	      return true;
	    case'r':
	      refy = absy + addy;
	      redraw = true;
	      break;
	    case '?':
	      d = 1;
	      // fall through
	    case '/':
	      direction = d;
	      // search in tracebuffer events
	      printf_statline("Search=%s", search_str);
	      cursor(LAST_LINE, 12+strlen(search_str));
	      get_string(search_str, sizeof(search_str), SPECIAL_ALL_PRINTABLE);
	      // fall through
	    case 'n':
		{
		  Mword n = search_tracebuffer(absy+addy, search_str,
					       direction);
		  if (n != (Mword)-1)
		    {
		      // found
		      if ((n < absy) || (n > absy+lines-1))
			{
			  // screen crossed
			  addy = 4;
			  absy = n - addy;
			  if (n < addy)
			    {
			      addy = n;
			      absy = 0;
			    }
			  if (absy > max_absy)
			    {
			      absy = max_absy;
			      addy = n - absy;
			    }
			  redraw = true;
			}
		      else
	      		addy = n - absy;
		    }
		}
	      printf_statline("T%73s", status);
	      break;
	    case KEY_ESC:
	      abort_command();
	      goto return_false;
	    default:
	      if (is_toplevel_cmd(c)) 
		{
		return_false:
		  show_tb_absy = absy;
		  show_tb_refy = refy;
		  show_tb_nr   = fst_nr-addy;
		  return false;
		}
	    }
	}
    }
}

// bt : ts->ebp
// bt thread xx.xxx 
// bt 0xxxxxxxxx
static 
void
Jdb::back_trace()
{
  unsigned address, ebp, eip1;
  L4_uid tid = current_tid; // default is current thread

  if (!x_get_32(&address, SPECIAL_T))
    {
      if (address == 0xffffffff)
	return;

      if (address == SPECIAL_T) // thread
	{ 
	  putstr(" thread:");
	  if (   !get_thread_id(&tid) 
	      || tid.is_invalid())
       	    return;
	}

      if (   tid.is_invalid()
	  || !check_thread(tid)
	  || !(threadid_t(&tid).lookup()->state()))
	{
	  printf(" Invalid thread\n");
	  return;
	}

      get_user_eip_ebp(tid, &eip1, &ebp);
    }
  else // use given address
    {
      eip1 = 0;
      ebp  = address;
    }

  printf("\n\nback trace (thread %x.%x, fp=%08x, eip=%08x):\n",
          tid.task(), tid.lthread(), ebp, eip1);

  if (tid.task() != 0)
    dump_backtrace(ebp, eip1, 0, tid.task(), false);
  
#ifdef CONFIG_NO_FRAME_PTR
  puts("  --kernel-bt-follows-- (don't trust wo. frame pointer!!)");
  dump_kernel_backtrace_wo_ebp(tid);
#else
  unsigned eip2;

  puts(" --kernel-bt-follows--");
  get_kernel_eip_ebp(tid, &eip1, &eip2, &ebp);

  dump_backtrace(ebp, eip1, eip2, tid.task(), true);
#endif
  putchar('\n');
}

static
void
Jdb::print_backtrace_item(int nr, unsigned addr, unsigned task)
{
  char symbol[60];

  printf(" %s#%d  %08x", (nr<10)?" ":"", nr, addr);

  if (Jdb_symbol::match_eip_to_symbol(addr, task, symbol, sizeof(symbol)))
    printf(" : %s", symbol);

  if (Jdb_lines::match_address_to_line_fuzzy(addr, task, 
					     symbol, sizeof(symbol)-1, 0))
    printf("\n                   %s", symbol);

  putchar('\n');
}

static
void
Jdb::dump_backtrace(unsigned ebp, unsigned eip1, unsigned eip2, 
		    unsigned task, bool in_kernel)
{
  for (int i=0; i<40 /* sanity check */; i++)
    {
      unsigned x, ebp_pa, ebp_1_pa;

      if (i > 1)
	{
	  if (  (ebp == 0)
	      ||(ebp == Kmem::mem_phys)
	      ||((ebp_pa   = k_lookup(ebp  , task)) == PAGE_INVALID)
	      ||((ebp_1_pa = k_lookup(ebp+4, task)) == PAGE_INVALID))
	    // invalid ebp -- leaving
	    return;
	  
	  ebp = *(unsigned*)ebp_pa;
	  x   = *(unsigned*)ebp_1_pa;

	  if (   ( in_kernel && (x<(unsigned)&_start || x>(unsigned)&_ecode))
	      || (!in_kernel && (x==0 || x>Kmem::mem_user_max)))
	    // no valid eip found -- leaving
	    return;
	}
      else if (i == 1)
	{
	  if (eip1 == 0)
	    continue;
	  x = eip1;
	}
      else
	{
	  if (eip2 == 0)
	    continue;
	  x = eip2;
	}

      print_backtrace_item(i, x, task);
    }
}

static inline NOEXPORT
unsigned
Jdb::get_state(L4_uid tid)
{
  return threadid_t(&tid).lookup()->state();
}

static
void
Jdb::get_user_eip_ebp(L4_uid tid, Mword *eip, Mword *ebp)
{ 
  if (tid.task() == 0)
    {
      // kernel thread doesn't execute (we hope so :-)) user level code
      *ebp = *eip = 0;
      return;
    }

  Thread *t = threadid_t(&tid).lookup();
  Mword ksp = (Mword) t->kernel_sp;
  Mword tcb_next = (ksp & ~(Config::thread_block_size-1))
		 + Config::thread_block_size;
  Mword *ktop = (Mword *)tcb_next;
  Guessed_thread_state state = guess_thread_state(t);

  if (state == s_ipc)
    {
      // If thread is in IPC, EBP lays on the stack. EBP location is by now
      // dependent from ipc-type, maybe different for other syscalls, maybe
      // unstable during calls but only for user EBP.
      Mword esp = ktop[-2];

      if (esp < Kmem::mem_user_max)
	{
	  if ((esp = k_lookup(esp, tid.task())) == PAGE_INVALID)
	    {
	      printf("\n esp page invalid");
	      *ebp = *eip = 0;
	      return;
	    }
	}

#ifdef CONFIG_ABI_X0
      // see ipc xadaption bindings: ebp lays at [esp+4]
      *ebp = ((Mword *)esp)[1];
#else
      // see ipc bindings: ebp lays at [esp]
      *ebp = ((Mword *)esp)[0];
#endif
    }
  else if (state == s_pagefault)
    {
      *ebp = ktop[-6];
    }
  else if (state == s_slowtrap)
    {
      *ebp = ktop[-13];
    }
  else
    {
      // Thread is doing (probaly) no IPC currently so we guess the
      // user ebp by following the kernel ebp upwards. Some kernel
      // entry pathes (e.g. timer interrupt) push the ebp register
      // like gcc.
      *ebp = get_user_ebp_following_kernel_stack(tid);
    }

  if (*ebp < Kmem::mem_user_max)
    *ebp = k_lookup(*ebp, tid.task());

  *eip = ktop[-5];
}

static
unsigned
Jdb::get_user_ebp_following_kernel_stack(L4_uid tid)
{
#ifndef CONFIG_NO_FRAME_PTR
  unsigned ebp, dummy;
  unsigned task = tid.task();

  get_kernel_eip_ebp(tid, &dummy, &dummy, &ebp);

  for (int i=0; i<30 /* sanity check */; i++)
    {
      unsigned x, ebp_pa, ebp_1_pa;

      if (  (ebp == 0)
    	  ||(ebp == Kmem::mem_phys)
	  ||((ebp_pa   = k_lookup(ebp  , task)) == PAGE_INVALID)
	  ||((ebp_1_pa = k_lookup(ebp+4, task)) == PAGE_INVALID))
	// invalid ebp -- leaving
	return 0;

      ebp = *(unsigned*)ebp_pa;
      x   = *(unsigned*)ebp_1_pa;

      if ((x<(unsigned)&_start || x>(unsigned)&_ecode))
	{
	  if (x < Kmem::mem_user_max)
	    // valid user ebp found
	    return ebp;
	  else
	    // invalid ebp
	    return 0;
	}
    }
#else
  (void)tid;
#endif

  // no suitable ebp found
  return 0;
}

#ifndef CONFIG_NO_FRAME_PTR
static
void
Jdb::get_kernel_eip_ebp(L4_uid tid, 
			unsigned *eip1, unsigned *eip2, unsigned *ebp)
{
  if (tid == current_tid)
    {
      *ebp  = (unsigned)__builtin_frame_address(3);
      *eip1 = *eip2 = 0;
    }
  else
    {
      unsigned *ksp = (unsigned*) threadid_t(&tid).lookup()->kernel_sp;
      unsigned tcb  = Kmem::mem_tcbs + tid.gthread()*Context::size;
      unsigned tcb_next = tcb + Context::size;

      // search for valid ebp/eip
      for (int i=0; (unsigned)(ksp+i+1)<tcb_next-20; i++)
	{
	  if (ksp[i+1] >= (unsigned)&_start && ksp[i+1] <= (unsigned)&_ecode && 
	      ksp[i  ] >= tcb+0x180         && ksp[i  ] <  tcb_next-20 &&
	      ksp[i  ] > (unsigned)(ksp+i))
	    {
	      // valid frame pointer found
	      *ebp  = ksp[i  ];
	      *eip1 = ksp[i+1];
	      *eip2 = (tid != current_tid) ? ksp[0] : 0;
	      return;
	    }
	}
      *ebp = *eip1 = *eip2 = 0;
    }
}
#endif

#ifdef CONFIG_NO_FRAME_PTR
static
void
Jdb::dump_kernel_backtrace_wo_ebp(L4_uid tid)
{
  unsigned *ksp = (unsigned*) threadid_t(&tid).lookup()->kernel_sp;
  unsigned tcb  = Kmem::mem_tcbs + tid.gthread()*Context::size;
  unsigned tcb_next = tcb + Context::size;

  // search for valid eip
  for (int i=0, j=1; (unsigned)(ksp+i)<tcb_next-20; i++)
    {
      if (ksp[i] >= (unsigned)&_start && ksp[i] <= (unsigned)&_ecode)
	{
	  print_backtrace_item(j++, ksp[i], tid.task());
	}
    }
}
#endif

static
int
Jdb::get_set_msr(void)
{
  int c;
  Unsigned64 msr;
  Unsigned32 reg, high, low;

  if (!(Cpu::features() & FEAT_MSR))
    {
      printf("MSR not supported\n");
      return false;
    }

  c = putchar_verb(getchar());
  if ((c != 'r') && (c != 'w'))
    return false;

  if (!x_get_32(&reg, 0))
    return false;
  
  switch (c)
    {
    case 'w':
      if (VERBOSE)
	putstr(" high=");
      
      if (!x_get_32(&high, 0))
	return false;

      if (VERBOSE)
	putstr(" low=");

      if (!x_get_32(&low, 0))
	return false;

      msr = (Unsigned64)high << 32 | low;

      test_msr = 1;
      Cpu::wrmsr(msr, reg);
      test_msr = 0;

      // fall though
    case 'r':
      test_msr = 1;
      msr = Cpu::rdmsr(reg);
      test_msr = 0;
      
      printf(" => %08x:%08x\n", (Unsigned32)(msr >> 32), (Unsigned32)msr);
      break;
    }

  return true;
}

static
int
Jdb::get_set_adapter_memory(void)
{
  int c;
  unsigned reg, value, page, offs;

  c = putchar_verb(getchar());
  if ((c != 'r') && (c != 'w'))
    return false;

  // request address
  if (!x_get_32(&reg, 0))
    return false;

  // address is dword aligned
  reg &= ~3;

  page = establish_phys_mapping(reg, &offs);
  
  switch (c)
    {
    case 'w':
      if (VERBOSE)
	printf(" (%08x) value=", *(volatile unsigned*)(page + offs));
      
      if (!x_get_32(&value, 0))
	return false;

      *(volatile unsigned*)(page + offs) = value;

      // fall through
    case 'r':
      printf(" => %08x\n", *(volatile unsigned*)(page + offs));
      break;
    }

  return true;
}

static
void
Jdb::show_lbr_entry(const char *str, unsigned addr)
{
  char symbol[60];

  printf(str, addr);
  if (Jdb_symbol::match_eip_to_symbol(addr, 0, symbol, sizeof(symbol)))
    printf("(%s)", symbol);
}

static
void
Jdb::show_lbr(void)
{
  switch (int c=getchar())
    {
    case '+':
    case '-':
      lbr_active = (c == '+');
      putchar_verb(c);
      putchar_verb('\n');
      break;
    default:
      test_msr = 1;
      if (Cpu::lbr_type() == Cpu::LBR_P4)
	{
	  Unsigned64 msr;
	  Unsigned32 branch_tos;

	  msr = Cpu::rdmsr(0x1d7);
	  show_lbr_entry("\nbefore exc: %08x ", (Unsigned32)msr);
	  msr = Cpu::rdmsr(0x1d8);
	  show_lbr_entry("\n         => %08x ", (Unsigned32)msr);

	  msr = Cpu::rdmsr(0x1da);
	  branch_tos = (Unsigned32)msr;

	  for (int i=0, j=branch_tos & 3; i<4; i++)
	    {
	      j = (j+1) & 3;
	      msr = Cpu::rdmsr(0x1db+j);
	      show_lbr_entry("\nbranch/exc: %08x ", (Unsigned32)(msr >> 32));
	      show_lbr_entry("\n         => %08x ", (Unsigned32)msr);
	    }
	}
      else if (Cpu::lbr_type() == Cpu::LBR_P6)
	{
	  Unsigned64 msr;

	  msr = Cpu::rdmsr(0x1db);
	  show_lbr_entry("\nbranch: %08x ", (Unsigned32)msr);
	  msr = Cpu::rdmsr(0x1dc);
	  show_lbr_entry("\n     => %08x ", (Unsigned32)msr);
	  msr = Cpu::rdmsr(0x1dd);
	  show_lbr_entry("\n   int: %08x ", (Unsigned32)msr);
	  msr = Cpu::rdmsr(0x1de);
	  show_lbr_entry("\n     => %08x ", (Unsigned32)msr);
	}
      else
	{
	  printf("Last branch recording feature not available");
	}

      test_msr = 0;

      putchar('\n');
      break;
    }
}

static
void
Jdb::set_log_event(void)
{
#ifdef NO_LOG_EVENTS
  if (VERBOSE)
    printf("Log events disabled in configuration\n");
  abort_command();
#else
  int i;

  assert(LOG_EVENT_MAX_EVENTS == 16);

  if (last_cmd != 'O' && VERBOSE)
    {
      for (i=0; i<LOG_EVENT_MAX_EVENTS; i++)
	if (Jdb_tbuf::log_events[i])
	  {
	    printf("\n    [%x] %s:\033[K",
		   i, Jdb_tbuf::log_events[i]->get_name());
	    cursor(LAST_LINE, 28);
	    printf("%s",
		   Jdb_tbuf::log_events[i]->enabled() ? " ON" : "off");
	  }
	else
	  printf("\n    [%x] <free>", i);
      printf("\n");
    }

  if (VERBOSE)
    {
      cursor(LAST_LINE, 0);
      printf("  {0..%01x}{+|-}: ", LOG_EVENT_MAX_EVENTS-1);
    }

  switch (i = putchar_verb(getchar()))
    {
    case '0' ... '9':
      i -= '0';
      break;
    case 'a' ... 'f':
      i -= 'a' - 10;
      break;
    default:
      abort_command();
      return;
    }

  if (!Jdb_tbuf::log_events[i])
    {
      /* logpatch does not exist */
      abort_command();
      return;
    }

  switch (int c=getchar())
    {
    case '+':
    case '-':
      assert(i == Jdb_tbuf::log_events[i]->get_type());
      Jdb_tbuf::log_events[i]->enable(c=='+');
      break;
    }

#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
  // Logging of context switch and ipc shortcut failed/succeeded is not
  // implemented in Assembler shortcut. So if we want to log such events,
  // we have to use the C shortcut.
  if (Jdb_tbuf::log_events[0]->enabled() || 
      Jdb_tbuf::log_events[1]->enabled())
    Thread::cpath_ipc = true;
  else
    Thread::cpath_ipc = false;
  Thread::set_ipc_vector();
#endif

  if (VERBOSE)
    {
      cursor(LAST_LINE-LOG_EVENT_MAX_EVENTS+i, 28);
      printf("%s", Jdb_tbuf::log_events[i]->enabled() ? " ON" : "off");
    }
#endif
}

