INTERFACE [ia32,ux]:

#include "l4_types.h"

IMPLEMENTATION [ia32,ux]:

#include <cstdio>
#include "entry_frame.h"
#include "jdb.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "l4_types.h"
#include "mem_layout.h"
#include "push_console.h"
#include "simpleio.h"
#include "static_init.h"
#include "thread.h"
#include "types.h"
#include "cpu.h"

class Jdb_tcb : public Jdb_module
{
  static L4_uid	 threadid;
  static Address address;
  static char    first_char;
  static char    auto_tcb;
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

  inline bool valid(Address base)
  { 
    return _base == base && 
           _offs <= Config::thread_block_size-sizeof(Mword); 
  }

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
bool
Jdb_tcb_ptr::in_backtrace(Address bt_start, Address tcb)
{
  if (bt_start)
    {
      if (!Config::Have_frame_ptr)
	return Mem_layout::in_kernel_code(value());

      Jdb_tcb_ptr ebp(bt_start);

      for (;;)
	{
	  Jdb_tcb_ptr eip(ebp.addr()+4);

	  if (!Mem_layout::in_kernel_code(eip.value()))
	    return false;
	  if (ebp.addr()+4 == addr())
	    return true;
	  if (ebp.addr() == 0 || !Jdb_tcb_ptr(ebp.value()).valid(tcb))
	    return false;

	  ebp = ebp.value();
	}
    }
  return false;
}


PUBLIC
Jdb_tcb::Jdb_tcb()
  : Jdb_module("INFO")
{
  static Jdb_handler enter(at_jdb_enter);

  Jdb::jdb_enter.add(&enter);
}

static void
Jdb_tcb::at_jdb_enter()
{
  // the final 0xff prevents attaching a terminating KEY_RETURN keystroke
  static Unsigned8 const tcb_cmd[3] = { 't', ' ', 0xff };

  if (auto_tcb)
    {
      // clear any keystrokes in queue
      Jdb::set_next_cmd(0);
      Push_console::push(tcb_cmd, sizeof(tcb_cmd), 0);
    }
}

static
Address
Jdb_tcb::search_bt_start(Address tcb, Mword *ksp, bool is_current_thread)
{
  if (!Config::Have_frame_ptr)
    return 1;

  if (is_current_thread)
    return (Address)__builtin_frame_address(6);

  Address tcb_next = tcb + Context::size;

  for (int i=0; (Address)(ksp+i+1)<tcb_next-20; i++)
    {
      if (Mem_layout::in_kernel_code(ksp[i+1]) &&
	  ksp[i] >= tcb + 0x180 &&
	  ksp[i] <  tcb_next-20 &&
	  ksp[i] >  (Address)(ksp+i))
	return (Address)(ksp+i);
    }

  return 0;
}

static
void
Jdb_tcb::print_entry_frame_regs(Task_num task)
{
  Jdb_entry_frame *ef = Jdb::get_entry_frame();
  int from_user       = ef->from_user();
  Address disass_addr = ef->ip();

  // registers, disassemble
  printf("EAX=%08lx  ESI=%08lx  DS=%04lx\n"
	 "EBX=%08lx  EDI=%08lx  ES=%04lx     ",
	 ef->_eax, ef->_esi, ef->_ds & 0xffff, ef->_ebx, ef->_edi, ef->_es & 0xffff);

  if (jdb_disasm_one_line != 0)
    {
      putstr(Jdb::esc_emph);
      jdb_disasm_one_line(-40, disass_addr, 0, from_user ? task : 0);
      putstr("\033[m");
    }

  printf("ECX=%08lx  EBP=%08lx  GS=%04lx     ",
	 ef->_ecx, ef->_ebp, ef->_gs & 0xffff);

  if (jdb_disasm_one_line != 0)
    jdb_disasm_one_line(-40, disass_addr, 0, from_user ? task : 0);

  printf("EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	 "trap %ld (%s), error %08lx, from %s mode\n"
	 "CS=%04lx  EIP=%s%08lx\033[m  EFlags=%08lx\n",
	 ef->_edx, ef->sp(), ef->ss() & 0xffff,
	 ef->_trapno, Cpu::exception_string(ef->_trapno), ef->_err,
	 from_user ? "user" : "kernel",
	 ef->cs() & 0xffff, Jdb::esc_emph, ef->ip(), ef->flags());

  if (ef->_trapno == 14)
    printf("page fault linear address "L4_PTR_FMT"\n", ef->_cr2);
}

static void
Jdb_tcb::info_thread_state(Thread *t, Jdb::Guessed_thread_state state)
{
  Jdb_tcb_ptr p((Address)t->get_kernel_sp());
  int sub = 0;

  switch (state)
    {
    case Jdb::s_ipc:
    case Jdb::s_syscall:
      printf("EAX=%08lx  ESI=%08lx\n"
	     "EBX=%08lx  EDI=%08lx\n"
    	     "ECX=%08lx  EBP=%08lx\n"
	     "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	     "in %s (user level registers)",
	     p.top_value( -6), p.top_value(-10),
	     p.top_value( -8), p.top_value( -9),
	     p.top_value(-12), p.top_value( -7),
	     p.top_value(-11), p.top_value( -2), p.top_value(-1) & 0xffff,
	     state == Jdb::s_ipc ? "ipc" : "syscall");
      break;
    case Jdb::s_user_invoke:
      printf("EAX=00000000  ESI=00000000\n"
	     "EBX=00000000  EDI=00000000\n"
	     "ECX=00000000  EBP=00000000\n"
	     "EDX=00000000  ESP=%08lx  SS=%04lx\n"
	     "invoking user the first time (user level registers)",
	     p.top_value(-2), p.top_value(-1) & 0xffff);
      break;
    case Jdb::s_fputrap:
      printf("EAX=%08lx\n"
	     "\n"
    	     "ECX=%08lx\n"
	     "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	     "in exception #0x07 (user level registers)",
	     p.top_value(-7), p.top_value(-8),
	     p.top_value(-9), p.top_value(-2), p.top_value(-1) & 0xffff);
      break;
    case Jdb::s_pagefault:
      printf("EAX=%08lx\n"
	     "\n"
    	     "ECX=%08lx  EBP=%08lx\n"
	     "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	     "in page fault, error %08lx (user level registers)\n"
	     "\n"
	     "page fault linear address %08lx",
	     p.top_value( -7), p.top_value( -8), p.top_value(-6), 
	     p.top_value( -9), p.top_value( -2), p.top_value(-1) & 0xffff, 
	     p.top_value(-11), p.top_value(-10));
      break;
    case Jdb::s_interrupt:
      printf("EAX=%08lx\n"
	     "\n"
    	     "ECX=%08lx\n"
	     "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	     "in interrupt #0x%02lx (user level registers)",
	     p.top_value(-6), p.top_value(-8),
	     p.top_value(-7), p.top_value(-2), p.top_value(-1) & 0xffff,
	     p.top_value(-9));
      break;
    case Jdb::s_timer_interrupt:
      if (Config::Have_frame_ptr)
	sub = -1;
      printf("EAX=%08lx\n"
	     "\n"
    	     "ECX=%08lx\n"
	     "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
	     "in timer interrupt (user level registers)",
	     p.top_value(-6-sub), p.top_value(-8-sub),
	     p.top_value(-7-sub), p.top_value(-2), p.top_value(-1) & 0xffff);
      break;
    case Jdb::s_slowtrap:
      printf("EAX=%08lx  ESI=%08lx  DS=%04lx\n"
             "EBX=%08lx  EDI=%08lx  ES=%04lx\n"
             "ECX=%08lx  EBP=%08lx  GS=%04lx\n"
             "EDX=%08lx  ESP=%08lx  SS=%04lx\n"
             "in exception %ld, error %08lx (user level registers)",
	     p.top_value( -8), p.top_value(-14), p.top_value(-18) & 0xffff, 
	     p.top_value(-11), p.top_value(-15), p.top_value(-19) & 0xffff,
	     p.top_value( -9), p.top_value(-13), p.top_value(-17) & 0xffff,
	     p.top_value(-10), p.top_value( -2), p.top_value( -1) & 0xffff,
	     p.top_value( -7), p.top_value( -6));
      break;
    case Jdb::s_unknown:
      break;
    }
}

PUBLIC static
Jdb_module::Action_code
Jdb_tcb::show(L4_uid tid, int level)
{
new_tcb:

  Thread *t              = Thread::id_to_tcb(tid);
  Thread *t_current      = Jdb::get_current_active();
  bool is_current_thread;
  bool redraw_screen     = true;
  Jdb_entry_frame *ef    = Jdb::get_entry_frame();
  Address bt_start       = 0;

  if (tid.is_invalid() && !t_current)
    {
      print_regs_invalid_tid();
      return NOTHING;
    }

  if (!t)
    t = t_current;

  is_current_thread = t == t_current;

  if (!t->is_mapped())
    {
      puts(" Invalid thread");
      return NOTHING;
    }

  Address ksp  = is_current_thread ? ef->ksp()
				   : (Address)t->get_kernel_sp();
  Address tcb  = (Address)context_of((void*)ksp);
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

  putstr("\t\t\t\t\tprsnt lnk: ");
  if (t->present_next)
    t->present_next->print_uid(3);
  else
    putstr("---.--");
  if (t->present_prev)
    t->present_prev->print_uid(4);
  else
    putstr(" ---.--");
  putchar('\n');

  if (is_current_thread)
    print_entry_frame_regs(t->d_taskno());

  else if (t->d_taskno() != 0)
    {
      info_thread_state(t, Jdb::guess_thread_state(t));
      Jdb::cursor(15, 1);
      printf("CS=%04lx  EIP=%s%08lx\033[m  EFlags=%08lx kernel ESP=%08lx",
	      current.top_value(-4) & 0xffff, Jdb::esc_emph, 
	      current.top_value(-5), current.top_value(-3), ksp);
      if (jdb_disasm_one_line != 0)
	{
	  // disassemble two lines at EIP
	  Address disass_addr = current.top_value(-5);
	  Jdb::cursor(11,41);
	  putstr(Jdb::esc_emph);
	  jdb_disasm_one_line(-40, disass_addr, 0, t->d_taskno());
	  putstr("\033[m");
	  Jdb::cursor(12,41);
	  jdb_disasm_one_line(-40, disass_addr, 0, t->d_taskno());
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

      if (p.valid(tcb))
	{
	  printf("    %03lx ", p.addr() & 0xfff);
	  for (int x=0; x <= 7; x++, p+=1)
	    {
	      // It is possible that we have an invalid kernel_sp
	      // (in result of a kernel error). Handle it smoothly.
	      if (Kmem::virt_to_phys((const void*)p.addr()) == (Address)-1)
		{
		  putstr(" ........");
		  continue;
		}

	      char const *s1="", *s2 = "";
    	      if (p.is_user_value())
		{
		  s1 = Jdb::esc_iret;
		  s2 = "\033[m";
		}
	      else if (p.in_backtrace(bt_start, tcb))
		{
		  s1 = Jdb::esc_bt;
		  s2 = "\033[m";
		}
	      printf(" %s%08lx%s", s1, p.value(), s2);
	    }
	  putchar('\n');
	}
      else
	puts("\033[K");
    }

  Jdb::printf_statline("tcb", "<Tab>=backtrace e=edit u=disasm", "_");

  for (bool redraw=false; ; )
    {
      current.offs(absy*32 + addy*32 + addx*4);
      if (current.valid(tcb))
	{
	  Jdb::cursor(addy+17, 1);
	  printf("%08lx", current.addr());
	  Jdb::cursor(addy+17, 9*addx+10);
	  printf("%s%08lx\033[m", Jdb::esc_emph, current.value());
	}
      Jdb::cursor(addy+17, 9*addx+10);
      int c=Jdb_core::getchar();
      if (current.valid(tcb))
	{
	  Jdb::cursor(addy+17, 1);
	  printf("    %03lx ", current.addr() & 0xfff);
	  Jdb::cursor(addy+17, 9*addx+10);
	  printf("%s%08lx\033[m", 
	      current.is_user_value() 
	          ? Jdb::esc_iret 
		  : current.in_backtrace(bt_start, tcb)
		      ? Jdb::esc_bt
		      : "",
	      current.value());
	}
      Jdb::cursor(Jdb_screen::height(), 6);

      if ((c == KEY_CURSOR_HOME) && (level > 0))
	return GO_BACK;

      Mword   lines    = Jdb_screen::height() - 17;
      Mword   cols     = 8;
      Address max_absy = Config::thread_block_size/32 - lines;
      if (lines > Config::thread_block_size/32)
	max_absy = 0;
      if (lines > Config::thread_block_size/32 - absy)
	lines = Config::thread_block_size/32 - absy;

      if (!Jdb::std_cursor_key(c, cols, lines, max_absy, 
			       &absy, &addy, &addx, &redraw))
	{
	  Task_num task = current.is_user_value() ? t->d_taskno() : 0;

	  switch (c)
	    {
	    case KEY_RETURN:
	      if (jdb_dump_addr_task && current.valid(tcb))
		{
		  if (!jdb_dump_addr_task(current.value(), task, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case KEY_TAB:
	      bt_start = search_bt_start(tcb, (Mword*)ksp, is_current_thread);
	      redraw = true;
    	      break;
	    case ' ':
	      if (jdb_disasm_addr_task && current.valid(tcb))
		{
		  if (!jdb_disasm_addr_task(current.value(), task, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case 'u':
	      if (jdb_disasm_addr_task && current.valid(tcb))
		{
		  Jdb::printf_statline("tcb", "<CR>=disassemble here",
				       "u[address=%08lx task=%x] ",
					current.value(), task);
		  int c1 = Jdb_core::getchar();
		  if ((c1 != KEY_RETURN) && (c1 != ' '))
		    {
		      Jdb::printf_statline("tcb", 0, "u");
		      Jdb::execute_command("u", c1);
		      return NOTHING;
		    }

		  if (!jdb_disasm_addr_task(current.value(), task, level+1))
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
	      if (current.valid(tcb))
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
		      Jdb::printf_statline("tcb", 0, "edit <%08lx> = %08lx",
					  current.addr(), current.value());
		      Jdb::cursor(addy+17, 9*addx+10);
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
		      // edit registers
		      char reg = -1;
		      Mword *reg_ptr=0;
		      unsigned x=0, y=0;

		      Jdb::cursor(addy+17, 9*addx+10);
		      printf("%08lx", current.value());

		      Jdb::printf_statline("tcb", 0, "edit register "
					"e{ax|bx|cx|dx|si|di|sp|bp|ip|fl}: ");
		      Jdb::cursor(Jdb_screen::height(), 53);
		      if (!Jdb::get_register(&reg))
			goto no_change;

		      switch (reg)
			{
			case  1: x= 4; y= 9; reg_ptr = &ef->_eax; break;
			case  2: x= 4; y=10; reg_ptr = &ef->_ebx; break;
			case  3: x= 4; y=11; reg_ptr = &ef->_ecx; break;
			case  4: x= 4; y=12; reg_ptr = &ef->_edx; break;
			case  5: x=18; y=11; reg_ptr = &ef->_ebp; break;
			case  6: x=18; y= 9; reg_ptr = &ef->_esi; break;
			case  7: x=18; y=10; reg_ptr = &ef->_edi; break;
			case  8: x=13; y=14; reg_ptr = &ef->_eip; break;
			case  9: // we have no esp if we come from kernel
				 if (!ef->from_user())
				   goto no_change;
				 x=18; y=12; reg_ptr = &ef->_esp; break;
			case 10: x=35; y=12; reg_ptr = &ef->_eflags; 
				 break;
			default: goto no_change;
			}

		      Jdb::cursor(y+1, x+1);
		      putstr("        ");
		      Jdb::printf_statline("tcb", 0, "edit %s = %08lx", 
					   Jdb_screen::Reg_names[reg-1],
					   *reg_ptr);
		      Jdb::cursor(y+1, x+1);
		      if (Jdb_input::get_mword(&value, 8, 16))
			*reg_ptr = value;
		      redraw_screen = true;
		      break;
		    }
	    no_change:
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
		fmt       = " addr=%8lx => ";
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

//----------------------------------------------------------------------------
IMPLEMENTATION [v2,x0]:

PRIVATE static
void
Jdb_tcb::print_regs_invalid_tid()
{
  const Mword mask 
    = (Config::thread_block_size * Mem_layout::max_threads()) - 1;
  const Mword tsksz = Config::thread_block_size * L4_uid::threads_per_task();

  LThread_num task = ((Address)Jdb::get_thread() & mask) / tsksz;

  putchar('\n');
  print_entry_frame_regs (task);
}

static inline
void
Jdb_tcb::print_thread_uid_raw(Thread *t)
{
  printf(" <%08x>         ", (Unsigned32)(t->id().raw()));
}
