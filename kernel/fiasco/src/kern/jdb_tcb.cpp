INTERFACE[ia32,ux,arm]:

#include "l4_types.h"

IMPLEMENTATION[ia32,ux,arm]:

#include <cstdio>
#include <cstring>

#include "entry_frame.h"
#include "jdb.h"
#include "jdb_handler_queue.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_util.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "l4_types.h"
#include "push_console.h"
#include "simpleio.h"
#include "static_init.h"
#include "thread.h"
#include "thread_state.h"
#include "types.h"

class Jdb_tcb : public Jdb_module
{
  static L4_uid	 threadid;
  static Address address;
  static char    first_char;
  static char    auto_tcb;

private:
  static void print_entry_frame_regs();
};

class Jdb_tcb_ptr
{
public:
  Jdb_tcb_ptr(Address addr)
    : _base(addr & ~(Config::thread_block_size-1)),
      _offs(addr &  (Config::thread_block_size-1))
  {}

  Jdb_tcb_ptr(Jdb_tcb_ptr &p)
    : _base(p.base()), _offs(p.offs())
  {}

  inline bool valid()
  { return _offs <= Config::thread_block_size-sizeof(Mword); }

  bool operator > (int offs)
  {
    return offs < 0 ? _offs > Config::thread_block_size + offs*sizeof(Mword)
		    : _offs > offs*sizeof(Mword);
  }

  Jdb_tcb_ptr &operator += (int offs)
  { _offs += offs*sizeof(Mword); return *this; }

  inline Address addr()
  { return _base + _offs; }

  inline Mword value()
  { return *(Mword*)(_base + _offs); }

  inline void value(Mword v)
  { *(Mword*)(_base + _offs) = v; }

  inline bool is_user_value()
  { return _offs >= Config::thread_block_size - 5*sizeof(Mword); }

  inline Mword top_value(int offs)
  { return *((Mword*)(_base + Config::thread_block_size) + offs); }

  inline Address base()
  { return _base; }

  inline Address offs()
  { return _offs; }

  inline void offs(Address offs)
  { _offs = offs; }

private:
  Address  _base;
  Address  _offs;
};


L4_uid	Jdb_tcb::threadid;
Address Jdb_tcb::address;
char	Jdb_tcb::first_char;
char	Jdb_tcb::auto_tcb;


// available from jdb_disasm module
extern int jdb_disasm_one_line (int len, Address &addr,
				int show_symbols, Task_num task)
  __attribute__((weak));
// available from jdb_disasm module
extern int jdb_disasm_addr_task (Address addr, Task_num task, int level)
  __attribute__((weak));
// available from jdb_dump module
extern int jdb_dump_addr_task (Address addr, Task_num task, int level)
  __attribute__((weak));

PUBLIC
Jdb_tcb::Jdb_tcb()
{
  static Jdb_handler enter(at_jdb_enter);

  Jdb::jdb_enter.add(&enter);
}

static void
Jdb_tcb::at_jdb_enter()
{
  static Unsigned8 const tcb_cmd[2] = { 't', ' ' };
  if (auto_tcb)
    {
      // clear any keystrokes in queue
      Jdb::set_next_cmd(0);
      Push_console::push(tcb_cmd, 2);
    }
}

#if 0
static void
Jdb_tcb::info_thread_state(Thread *t, Jdb::Guessed_thread_state state)
{
  Jdb_tcb_ptr p((Address)t->get_kernel_sp());
  int sub = 0;

  switch (state)
    {
    case Jdb::s_ipc:
      if (state == Jdb::s_ipc)
	{
	  printf("EAX=%08x  ESI=%08x\n"
		 "EBX=%08x  EDI=%08x\n"
		 "ECX=%08x  EBP=%08x\n"
		 "EDX=%08x  ESP=%08x  SS=%04x\n"
		 "in ipc (user level registers)",
		 p.top_value( -6), p.top_value(-10),
		 p.top_value( -8), p.top_value( -9),
		 p.top_value(-12), p.top_value( -7),
		 p.top_value(-11), p.top_value( -2), p.top_value(-1) & 0xffff);
	}
      break;
    case Jdb::s_fputrap:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in exception #0x07 (user level registers)",
	     p.top_value(-7), p.top_value(-8),
	     p.top_value(-9), p.top_value(-2), p.top_value(-1) & 0xffff);
      break;
    case Jdb::s_pagefault:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x  EBP=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in page fault, error %08x (user level registers)\n"
	     "\n"
	     "page fault linear address %08x",
	     p.top_value( -7), p.top_value( -8), p.top_value(-6), 
	     p.top_value( -9), p.top_value( -2), p.top_value(-1) & 0xffff, 
	     p.top_value(-10), p.top_value(-11));
      break;
    case Jdb::s_interrupt:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in interrupt #0x%02x (user level registers)",
	     p.top_value(-6), p.top_value(-8),
	     p.top_value(-7), p.top_value(-2), p.top_value(-1) & 0xffff,
	     p.top_value(-9));
      break;
    case Jdb::s_timer_interrupt:
#ifndef CONFIG_NO_FRAME_PTR
      sub = -1;
#endif
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in timer interrupt (user level registers)",
	     p.top_value(-6-sub), p.top_value(-8-sub),
	     p.top_value(-7-sub), p.top_value(-2), p.top_value(-1) & 0xffff);
      break;
    case Jdb::s_slowtrap:
      printf("EAX=%08x  ESI=%08x  DS=%04x\n"
             "EBX=%08x  EDI=%08x  ES=%04x\n"
             "ECX=%08x  EBP=%08x  GS=%04x\n"
             "EDX=%08x  ESP=%08x  SS=%04x\n"
             "in exception %d, error %08x (user level registers)",
	     p.top_value( -9), p.top_value(-15), p.top_value(-17) & 0xffff, 
	     p.top_value(-12), p.top_value(-16), p.top_value(-18) & 0xffff,
	     p.top_value(-10), p.top_value(-14), p.top_value(-19) & 0xffff,
	     p.top_value(-11), p.top_value( -2), p.top_value( -1) & 0xffff,
	     p.top_value( -8), p.top_value( -7));
      break;
    case Jdb::s_unknown:
      break;
    }
}
#endif 

PUBLIC static
Jdb_module::Action_code
Jdb_tcb::show(L4_uid tid, int level)
{
new_tcb:

  Thread *t              = Thread::id_to_tcb(tid);
  Thread *t_current      = Jdb::get_current_active();
  bool is_current_thread;
  bool redraw_screen     = true;

  if (tid.is_invalid() && !t_current)
    {
#if 0
      const Mword mask 
	= (Config::thread_block_size * Mem_layout::max_threads()) - 1;
      const Mword tsksz = Config::thread_block_size*L4_uid::threads_per_task();
      LThread_num task = ((Address)Jdb::get_thread() & mask) / tsksz;
#endif
      putchar('\n');
      print_entry_frame_regs();
      return NOTHING;
    }

  if (!t)
    t = t_current;

  is_current_thread = t == t_current;

  if (!t->is_valid())
    {
      puts(" Invalid thread");
      return NOTHING;
    }

  Address ksp  = (Address)t->get_kernel_sp();
  Jdb_tcb_ptr current(ksp);
  Address absy =  current.offs() / 32;
  Address addy = 0;
  Address addx = (current.offs() & 31) / 4;

  if (level==0)
    {
      Jdb::clear_screen(Jdb::FANCY);
      redraw_screen = false;
    }

whole_screen:
  
  if (redraw_screen)
    {
      Jdb::clear_screen(Jdb::NOFANCY);
      redraw_screen = false;
    }
 
  char time_str[12];

  putstr("thread: ");
  t->print_uid(3);
  print_thread_uid_raw(t);

  printf("\tprio: %02x  mcp: %02x  mode: %s\n",
      t->sched()->prio(), t->mcp(),
      t->mode() & Context::Periodic  ?
      t->mode() & Context::Nonstrict ? "Per (IRT)" : "Per (SP)" : "Con");

  printf("state: %03lx ", t->state());
  t->print_state_long();

  putstr("\n\nwait for: ");
  if (!t->partner())
    putstr("  *.** ");
  else
    t->print_partner(3);

  putstr("   polling: ");
  t->print_snd_partner(3);

  putstr("\trcv descr: ");
 
  if (t->state() & Thread_ipc_receiving_mask)
    printf("%08lx", t->rcv_regs()->rcv_desc().raw());
  else
    putstr("        ");

  putstr("\n"
         "lcked by: ");
  Thread::lookup(t->thread_lock()->lock_owner())->print_uid(3);

  putstr("\t\t\ttimeout  : ");
  if (t->_timeout && t->_timeout->is_set())
    {
      Signed64 diff = (t->_timeout->get_timeout()) * 1000;
      if (diff < 0)
	strcpy(time_str, "over");
      else
	Jdb::write_ll_ns(diff, time_str,
	                 11 < sizeof(time_str)-1 ? 11 : sizeof(time_str)-1,
	                 false);
      printf("%-13s", time_str);
    }

  putstr("\ncpu time: ");
  Jdb::write_ll_ns(t->consumed_time()*1000, time_str, 
                   11 < sizeof(time_str) ? 11 : sizeof(time_str), false);
  printf("%-13s", time_str);

  printf("\t\t\ttimeslice: %llu/%llu %cs\n"
         "pager\t: ",
	  t->sched()->left(), t->sched()->quantum(), Config::char_micro);
  t->_pager->print_uid(3);

  putstr("\tcap: ");
  t->_cap_handler->print_uid(3);

  putstr("\npreemptr: ");
  Thread::lookup(context_of(t->preemption()->receiver()))->print_uid(3);

  printf("\t%smonitored", t->space()->task_caps_enabled() ? "" : "not ");

  putstr("\tready lnk: ");
  if (t->state() & Thread_ready)
    {
      if (t->_ready_next)
	Thread::lookup(t->_ready_next)->print_uid(3);
      else if (is_current_thread)
	putstr(" ???.??");
      else
	putstr("\033[31;1m???.??\033[m");
      if (t->_ready_prev)
	Thread::lookup(t->_ready_prev)->print_uid(4); 
      else if (is_current_thread)
	putstr(" ???.??");
      else
      	putstr(" \033[31;1m???.??\033[m");
      putchar('\n');
    }
  else
    puts("---.-- ---.--");

  putstr("\t\t\t\t\tprsent lnk: ");
  if (t->present_next)
    t->present_next->print_uid(3);
  else
    putstr("---.--");
  if (t->present_prev)
    t->present_prev->print_uid(4);
  else
    putstr(" ---.--");
  putchar('\n');


//  printf("last_irq = %08lx\n", t->last_irq);

  if (is_current_thread)
    print_entry_frame_regs();

  else if (t->id().task() != 0)
    {
      //info_thread_state(t, Jdb::guess_thread_state(t));
      Jdb::cursor(12, 1);
      printf("PC=%s%08lx\033[m USP=%08lx\n",
	     Jdb::esc_emph, current.top_value(-1), current.top_value(-4));
      printf("[0] %08lx %08lx %08lx %08lx [4] %08lx %08lx %08lx %08lx\n",
             current.top_value(-18), current.top_value(-17),
             current.top_value(-16), current.top_value(-15),
             current.top_value(-14), current.top_value(-13),
             current.top_value(-12), current.top_value(-11));
      printf("[8] %08lx %08lx %08lx %08lx [c] %08lx %08lx %08lx %08lx\n",
             current.top_value(-10), current.top_value(-9),
             current.top_value(-8),  current.top_value(-7),
             current.top_value(-6),  current.top_value(-4),
             current.top_value(-3),  current.top_value(-1));
#if 0
      printf("CS=%04x  EIP=%s%08x\033[m  EFlags=%08x kernel ESP=%08x",
      Jdb::cursor(15, 1);
      printf("CS=%04lx  EIP=%s%08lx\033[m  EFlags=%08lx kernel ESP=%08lx",
	      current.top_value(-4) & 0xffff, Jdb::esc_emph, 
	      current.top_value(-5), current.top_value(-3), ksp);
#endif
      if (jdb_disasm_one_line != 0)
	{
	  // disassemble two lines at EIP
	  Address disass_addr = current.top_value(-5);
	  putstr(Jdb::esc_emph);
	  Jdb::cursor(11,41);
	  jdb_disasm_one_line(-40, disass_addr, 0, t->id().task());
	  Jdb::cursor(12,41);
	  putstr("\033[m");
	  jdb_disasm_one_line(-40, disass_addr, 0, t->id().task());
	}
    }
  else
    {
      // kernel thread
      Jdb::cursor(15, 1);
      printf("kernel ESP=%08lx", ksp);
    }

dump_stack:

  Jdb::cursor(17, 1);

  // dump the stack from ksp bottom right to tcb_top
  Jdb_tcb_ptr p = current;
  p.offs(absy*32);

  for (unsigned int y=0; y<Jdb_screen::height()-17; y++)
    {
      Kconsole::console()->getchar_chance();

      if (p.valid())
	{
	  printf("    %03lx ", p.addr() & 0xfff);
	  for (int x=0; x <= 7; x++, p+=1)
	    {
	      // It is possible that we have an invalid kernel_sp
	      // (in result of an kernel error). Handle it smoothly.
	      if (Jdb_util::is_mapped((const void*)p.addr()))
    		printf(" %s%08lx%s", p.is_user_value() ? Jdb::esc_iret : "",
				     p.value(),
				     p.is_user_value() ? "\033[m" : "");
	      else
		putstr(" ........");
	    }
	  putchar('\n');
	}
      else
	puts("\033[K");
    }

  Jdb::printf_statline("tcb", "%63s",
		       "e=edit u=disasm r|p+p|n=ready/present next/prev");

  for (bool redraw=false; ; )
    {
      current.offs(absy*32 + addy*32 + addx*4);
      if (current.valid())
	{
	  Jdb::cursor(addy+17, 1);
	  printf("%08lx", current.addr());
	  Jdb::cursor(addy+17, 9*addx+10);
	  printf("%s%08lx\033[m", Jdb::esc_emph, current.value());
	}
      Jdb::cursor(addy+17, 9*addx+10);
      int c=Jdb_core::getchar();
      if (current.valid())
	{
	  Jdb::cursor(addy+17, 1);
	  printf("    %03lx ", current.addr() & 0xfff);
	  Jdb::cursor(addy+17, 9*addx+10);
	  printf("%s%08lx\033[m", current.is_user_value() ? Jdb::esc_iret : "", 
	                          current.value());
	}
      Jdb::cursor(Jdb_screen::height(), 6);

      if ((c == KEY_CURSOR_HOME) && (level > 0))
	return GO_BACK;

      Mword   lines    = Jdb_screen::height() - 17;
      Mword   cols     = Jdb_screen::Columns - 1;
      Address max_absy = Config::thread_block_size/32 - lines;
      if (lines > Config::thread_block_size/32)
	max_absy = 0;
      if (lines > Config::thread_block_size/32 - absy)
	lines = Config::thread_block_size/32 - absy;

      if (!Jdb::std_cursor_key(c, cols, lines, max_absy, 
			       &absy, &addy, &addx, &redraw))
	{
	  switch (c)
	    {
	    case KEY_RETURN:
	      if (jdb_dump_addr_task && current.valid())
		{
		  if (!jdb_dump_addr_task(current.value(),
	       		current.is_user_value() ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case ' ':
	      if (jdb_disasm_addr_task && current.valid())
		{
		  if (!jdb_disasm_addr_task(current.value(),
			current.is_user_value() ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case 'u':
	      if (jdb_disasm_addr_task && current.valid())
		{
		  Jdb::printf_statline("tcb", 0, "u[address=%08x task=%x] ",
					current.value(), t->id().task());
		  int c1 = Jdb_core::getchar();
		  if ((c1 != KEY_RETURN) && (c1 != ' '))
		    {
		      Jdb::printf_statline("tcb", 0, "u");
		      Jdb::execute_command("u", c1);
		      return NOTHING;
		    }

		  if (!jdb_disasm_addr_task(current.value(),
			current.is_user_value() ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case 'r': // ready-list
	      putstr("[n]ext/[p]revious in ready list?");
	      switch (Jdb_core::getchar())
		{
		case 'n':
		  if (t->_ready_next)
		    {
		      L4_uid new_tid = (Thread::lookup(t->_ready_next))->id();
		      if (!new_tid.is_invalid())
			{
			  tid = new_tid;
			  goto new_tcb;
			}
		    }
		  break;
		case 'p':
		  if (t->_ready_prev)
		    {
		      L4_uid new_tid = (Thread::lookup(t->_ready_prev))->id();
		      if (!new_tid.is_invalid())
			{
			  tid = new_tid;
			  goto new_tcb;
			}
		    }
		  break;
		}
	      break;
	    case 'p': // present-list or show_pages
	      putstr("[n]ext/[p]revious in present list?");
	      switch (c=Jdb_core::getchar()) 
		{
		case 'n':
		  if (t->present_next)
		    {
		      L4_uid new_tid = (Thread::lookup(t->present_next))->id();
		      if (!new_tid.is_invalid())
			{
			  tid = new_tid;
			  goto new_tcb;
			}
		    }
		  break;
		case 'p':
		  if (t->present_prev)
		    {
		      L4_uid new_tid = (Thread::lookup(t->present_prev))->id();
		      if (!new_tid.is_invalid())
			{
			  tid = new_tid;
			  goto new_tcb;
			}
		    }
		  break;
		default:
		  Jdb::execute_command("p", c);
		  return NOTHING;
		}
	      break;
	    case 'e': // edit memory/registers
	      if (current.valid())
		{
		  Mword value;
		  int c;

		  Jdb::cursor(addy+17, 9*addx+10);
		  printf("        ");
		  Jdb::printf_statline("tcb",
					is_current_thread
					  ? "<Space>=edit registers" : 0,
					"edit <%08lx> = %08lx",
				        current.addr(), current.value());
		  Jdb::cursor(addy+17, 9*addx+10);
		  c = Jdb_core::getchar();
		  if (c==KEY_ESC)
		    {
		      redraw = true;
		      break;
		    }
		  if (c != ' ' || !is_current_thread)
		    {
		      // edit memory
		      putchar(c);
		      Jdb::printf_statline("tcb", 0, "edit <%08x> = %08x", 
			  current.addr(), current.value());
		      Jdb::cursor(addy+17, 9*addx+6);
		      if (!Jdb_input::get_mword(&value, c, 16))
			{
			  Jdb::cursor(addy+17, 9*addx+10);
			  printf("%08lx", current.value());
			  break;
			}
		      else
			current.value(value);
		    }
		  else
		    {
#if 0
		      // edit registers
		      char reg = -1U;
		      unsigned *reg_ptr=0, x=0, y=0;

		      Jdb::cursor(addy+17, 9*addx+6);
		      printf("%08x", current.value());

		      Jdb::printf_statline("edit register "
				      "e{ax|bx|cx|dx|si|di|sp|bp|ip|fl}: ");
		      Jdb::cursor(Jdb_screen::height(), 53);
		      Jdb::get_register(&reg);

		      switch (reg)
			{
			case  1: x= 4; y= 9; reg_ptr = &ef->eax; break;
			case  2: x= 4; y=10; reg_ptr = &ef->ebx; break;
			case  3: x= 4; y=11; reg_ptr = &ef->ecx; break;
			case  4: x= 4; y=12; reg_ptr = &ef->edx; break;
			case  5: x=18; y=11; reg_ptr = &ef->ebp; break;
			case  6: x=18; y= 9; reg_ptr = &ef->esi; break;
			case  7: x=18; y=10; reg_ptr = &ef->edi; break;
			case  8: x=13; y=14; reg_ptr = &ef->eip; break;
			case  9: // we have no esp if we come from kernel
				 if (!ef->from_user())
				   goto no_change;
				 x=18; y=12; reg_ptr = &ef->esp; break;
			case 10: x=35; y=12; reg_ptr = &ef->eflags; 
				 break;
			default: goto no_change;
			}

		      Jdb::cursor(y+1, x+1);
		      putstr("        ");
		      Jdb::printf_statline("edit %s = %08x", 
					   Jdb::reg_names[reg-1], *reg_ptr);
		      Jdb::cursor(y+1, x+1);
		      if (Jdb_input::get_mword(&value, 8, 16))
			*reg_ptr = value;
		      redraw_screen = true;
#endif
		      break;
		    }
//	    no_change:
		  redraw = true;
		}
	      break;
	    case KEY_ESC:
	      Jdb::abort_command();
	      return NOTHING;
	    default:
	      if (Jdb::is_toplevel_cmd(c))
		return NOTHING;
	      break;
	    }
	}
      if (redraw_screen)
	goto whole_screen;
      if (redraw)
	goto dump_stack;
    }
} 

/* --- original L4 screen ------------------------------------------------------
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
------------------------------------------------------------------------------*/

PUBLIC
Jdb_module::Action_code
Jdb_tcb::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      if (args == &first_char)
	{
	  switch (first_char)
	    {
	      case '+':
	      case '-':
		printf("%c\n", first_char);
		auto_tcb = first_char == '+';
		break;
	      case '?':
		args      = &address;
		fmt       = " addr=%8x => ";
		putchar(first_char);
		return Jdb_module::EXTRA_INPUT;
	      default:
		args      = &threadid;
		fmt       = "%t";
		next_char = first_char;
		return Jdb_module::EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	}
      else if (args == &address)
	{
	  address &= ~(Config::thread_block_size-1);
      	  reinterpret_cast<Thread*>(address)->print_uid(3);
	  putchar('\n');
	}
      else
	show(threadid, 0);
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_tcb::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "t", "tcb", "%C",
	  "t[<threadid>]\tshow current/given thread control block (TCB)\n"
	  "t{+|-}\tshow current thread control block at Jdb every entry\n"
	  "t?\tshow the corresponding thread id to a TCB address",
	  &first_char },
    };
  return cs;
}

PUBLIC
int
Jdb_tcb::num_cmds() const
{ return 1; }

static Jdb_tcb jdb_tcb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

int
jdb_show_tcb(L4_uid tid, int level)
{ return Jdb_tcb::show(tid, level); }


IMPLEMENTATION[v2]:

static inline
void
Jdb_tcb::print_thread_uid_raw(Thread *t)
{
  printf(" <%08x %08x>",
      (Unsigned32)(t->id().raw() >> 32), (Unsigned32)(t->id().raw()));
}


IMPLEMENTATION[!v2]:

static inline
void
Jdb_tcb::print_thread_uid_raw(Thread *t)
{
  printf(" <%08x>         ", (Unsigned32)(t->id().raw()));
}
