INTERFACE:

#include "l4_types.h"


IMPLEMENTATION[ia32-ux]:

#include <cstdio>

#include "jdb.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "l4_types.h"
#include "push_console.h"
#include "simpleio.h"
#include "static_init.h"
#include "thread.h"
#include "threadid.h"
#include "types.h"

class Jdb_tcb : public Jdb_module
{
  static L4_uid	threadid;
  static char	first_char;
  static char	auto_tcb;
};


L4_uid	Jdb_tcb::threadid;
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
  static Jdb_at_entry enter(at_jdb_enter);

  Jdb::at_enter.add(&enter);
}

static void
Jdb_tcb::at_jdb_enter(Thread *)
{
  static Unsigned8 const tcb_cmd[2] = { 't', ' ' };
  if (auto_tcb)
    {
      // clear any keystrokes in queue
      Jdb::set_next_cmd(0);
      Push_console::push(tcb_cmd, 2, 0);
    }
}

static void
Jdb_tcb::print_entry_frame_regs(Task_num task, Address ksp)
{
  Jdb_entry_frame *ef = Jdb::get_entry_frame();
  int from_user       = ef->from_user();
  Address disass_addr = ef->eip;

  // registers, disassemble
  printf("EAX=%08x  ESI=%08x  DS=%04x\n"
	 "EBX=%08x  EDI=%08x  ES=%04x     ",
	 ef->eax, ef->esi, ef->ds & 0xffff, ef->ebx, ef->edi, ef->es & 0xffff);

  if (jdb_disasm_one_line != 0)
    {
      putstr(Jdb::esc_emph);
      jdb_disasm_one_line(-40, disass_addr, 0, task);
      putstr("\033[m");
    }

  printf("ECX=%08x  EBP=%08x  GS=%04x     ",
	 ef->ecx, ef->ebp, ef->gs & 0xffff);

  if (jdb_disasm_one_line != 0)
    jdb_disasm_one_line(-40, disass_addr, 0, task);

  printf("EDX=%08x  ESP=%08x  SS=%04x\n"
	 "trapno %d, error %08x, from %s mode\n"
	 "CS=%04x  EIP=%s%08x\033[m  EFlags=%08x kernel ESP=%08x\n",
	 ef->edx, ef->_get_esp(), ef->_get_ss(),
	 ef->trapno, ef->err, from_user ? "user" : "kernel",
	 ef->cs & 0xffff, Jdb::esc_emph, ef->eip, ef->eflags, ksp);

  if (ef->trapno == 14)
    printf("page fault linear address %08x\n", ef->cr2);
}

static void
Jdb_tcb::info_thread_state(Thread *t, Jdb::Guessed_thread_state state)
{ 
  Mword *ktop = (Mword*)((((Mword)t->kernel_sp) & ~0x7ff) + 0x800);
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
		 ktop[ -6], ktop[-10], ktop[ -8], ktop[ -9], 
		 ktop[-12], ktop[ -7], ktop[-11], ktop[ -2], 
		 ktop[ -1] & 0xffff);
	}
      break;
    case Jdb::s_fputrap:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in exception #0x07 (user level registers)",
	     ktop[-7], ktop[-8], ktop[-9], ktop[-2], ktop[-1] & 0xffff);
      break;
    case Jdb::s_pagefault:
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
    case Jdb::s_interrupt:
      printf("EAX=%08x\n"
	     "\n"
    	     "ECX=%08x\n"
	     "EDX=%08x  ESP=%08x  SS=%04x\n"
	     "in interrupt #0x%02x (user level registers)",
	     ktop[-6], ktop[-8], ktop[-7], ktop[-2], ktop[-1] & 0xffff,
	     ktop[-9]);
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
	     ktop[-6-sub], ktop[-8-sub], ktop[-7-sub], ktop[-2],
	     ktop[-1] & 0xffff);
      break;
    case Jdb::s_slowtrap:
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
    case Jdb::s_unknown:;
    }
}

PUBLIC static
Jdb_module::Action_code
Jdb_tcb::show(L4_uid tid, int level)
{
new_tcb:

  Threadid t_id(&tid);
  Thread *t              = Jdb::get_current_active();
  bool redraw_screen     = true;
  bool is_current_thread = true;
  Jdb_entry_frame *ef    = Jdb::get_entry_frame();

  if (tid == L4_uid::INVALID && !t)
    {
      const Mword mask 
	= (Config::thread_block_size * Kmem::info()->max_threads()) - 1;
      const Mword tsksz = Config::thread_block_size*L4_uid::threads_per_task();
      LThread_num task = ((Address)Jdb::get_thread() & mask) / tsksz;

      putchar('\n');
      print_entry_frame_regs(task, 0);
      return NOTHING;
    }

  if (t_id.is_valid() && (t_id.lookup() != t))
    {
      is_current_thread = false;
      t = t_id.lookup();
    }

  if (!t->is_valid())
    {
      puts(" Invalid thread");
      return NOTHING;
    }

  Address ksp            = (Mword)t->kernel_sp;
  Address tcb            = ksp & ~(Config::thread_block_size-1);
  Address absy           = (ksp - tcb) / 32;
  Address addy           = 0;
  Address addx           = (ksp & 31) / 4;
  Address current;

  if (level==0)
    {
      Jdb::fancy_clear_screen();
      redraw_screen = false;
    }
  
  
whole_screen:
  
  if (redraw_screen)
    {
      Jdb::cursor();
      for(unsigned int i=0; i<Jdb_screen::height()-1; i++)
	{
	  Jdb::clear_to_eol();
	  putchar('\n');
	}
      Jdb::cursor();
      redraw_screen = false;
    }
  
  char tid_buf[32];
#ifdef CONFIG_ABI_V2
  sprintf(tid_buf, "<%08x %08x>",
	  (Unsigned32)(t->id().raw() >> 32), (Unsigned32)(t->id().raw()));
#else
  sprintf(tid_buf, "<%08x>         ",
	  (Unsigned32)(t->id().raw()));
#endif
  printf("thread: %3x.%02x %s\t\t\tprio: %02x\tmcp: %02x\n",
	 t->id().task(), t->id().lthread(), tid_buf,
	 t->sched()->prio(), t->mcp());

  printf("state: %03x ", t->state());
  t->print_state_long();

  putstr("\n\nwait for: ");
  t->print_partner();

  putstr("   polling: ");
  t->print_send_partner();

  putstr("\trcv descr: ");
  
  if (t->state() & Thread_ipc_receiving_mask)
    printf("%08x", t->receive_regs()->rcv_desc().raw());
  else
    putstr("        ");

  putstr("   partner: ---.--\n"
         "reqst.to: ");
  Thread::lookup(t->thread_lock()->lock_owner())->print_uid(3);

  putstr("\t\t\ttimeout: ");
  if (t->_timeout && t->_timeout->is_set())
    {
      Signed64 diff = (t->_timeout->get_timeout()) * 1000;
      char buffer[14];
      int len;
      if (diff < 0)
	diff = 0;
      len = Jdb::write_ll_ns(diff, buffer, sizeof(buffer)-1, false);
      buffer[len] = '\0';
      printf("%s", buffer);
    }

  Unsigned64 cputime = t->sched()->get_total_cputime();
  printf("\ncpu time: %02x%08x timeslice: %02x/%02x\n"
         "pager\t: ",
	  (unsigned)(cputime >> 32), (unsigned)cputime,
	  t->sched()->ticks_left(), t->sched()->timeslice());
  t->_pager->print_uid(3);

  putstr("\npreemptr: ");
  t->_preempter->print_uid(3);

  putstr("\t\t\tready lnk : ");
  if (t->state() & Thread_running)
    {
      if (t->ready_next)
	Thread::lookup(t->ready_next)->id().print(3);
      else
	putstr("???.??");
      if (t->ready_prev)
	Thread::lookup(t->ready_prev)->id().print(4); 
      else
      	putstr(" ???.??");
      putchar('\n');
    }
  else
    puts("---.-- ---.--");

  putstr("\t\t\t\t\tprsent lnk: ");
  if (t->present_next)
    t->present_next->id().print(3);
  else
    putstr("---.--");
  if (t->present_prev)
    t->present_prev->id().print(4);
  else
    putstr(" ---.--");
  putchar('\n');

  if (is_current_thread)
    print_entry_frame_regs(t->id().task(), ksp);

  else if (t->id().task() != 0)
    {
      Mword *k_top = (Mword*)((ksp & ~0x7ff) + 0x800);
      info_thread_state(t, Jdb::guess_thread_state(t));
      Jdb::cursor(15, 1);
      printf("CS=%04x  EIP=%s%08x\033[m  EFlags=%08x kernel ESP=%08x",
	      k_top[-4] & 0xffff, Jdb::esc_emph, k_top[-5], k_top[-3], ksp);
      if (jdb_disasm_one_line != 0)
	{
	  // disassemble two lines at EIP
	  Address disass_addr = k_top[-5];
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
      printf("kernel ESP=%08x", ksp);
    }

dump_stack:

  Jdb::cursor(17, 1);

  // dump the stack from ksp bottom right to tcb_top
  Address p = tcb + absy*32;
  for (unsigned int y=0; y<Jdb_screen::height()-17; y++)
    {
      Kconsole::console()->getchar_chance();

      if (p < tcb + Config::thread_block_size)
	{
	  printf("%03x:", p & 0xfff);
	  for (int x=0; x <= 7; x++)
	    {
	      // It is possible that we have an invalid kernel_sp
	      // (in result of an kernel error). Handle it smoothly.
	      if (Kmem::virt_to_phys((const void*)p) != (Address)-1)
    		printf(" %s%08x%s", 
		      ((p & 0x7ff) >= 0x7ec) ? Jdb::esc_iret : "",
		      *(unsigned*)p,
		      ((p & 0x7ff) >= 0x7ec) ? "\033[m" : "");
	      else
		putstr(" ........");
	      p+=4;
	    }
	  putchar('\n');
	}
      else
	puts("\033[K");
    }

  Jdb::printf_statline("t%73s",
		       "e=edit u=disasm r|p+p|n=ready/present next/prev");

  for (bool redraw=false; ; )
    {
      current = tcb + absy*32 + addy*32 + addx*4;
      if ((current & 0x800) == (ksp & 0x800))
	{
	  Jdb::cursor(addy+17, 9*addx+6);
	  printf("%s%08x\033[m", Jdb::esc_emph, *(Mword*)current);
	}
      Jdb::cursor(addy+17, 9*addx+6);
      int c=getchar();
      if ((current & 0x800) == (ksp & 0x800))
	{
	  Jdb::cursor(addy+17, 9*addx+6);
	  printf("%s%08x\033[m",
		 ((current & 0x7ff) >= 0x7ec) ? Jdb::esc_iret : "", 
		 *(Mword*)current);
	}
      Jdb::cursor(Jdb_screen::height(), Jdb::LOGO);

      if ((c == KEY_CURSOR_HOME) && (level > 0))
	return GO_BACK;

      Mword   lines    = Jdb_screen::height() - 17;
      Address max_absy = Config::thread_block_size/32 - lines;
      if (Config::thread_block_size/32 < lines)
	max_absy = 0;
      if (lines > Config::thread_block_size/32 - absy)
	lines = Config::thread_block_size/32 - absy;

      if (!Jdb::std_cursor_key(c, lines, max_absy, 
			       &absy, &addy, &addx, &redraw))
	{
	  switch (c)
	    {
	    case KEY_RETURN:
	      if (jdb_dump_addr_task && ((current & 0x800) == (ksp & 0x800)))
		{
		  if (!jdb_dump_addr_task(*(Address*)current, 
					  ((current & 0x7ff) >= 0x7ec) 
					     ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case ' ':
	      if (jdb_disasm_addr_task && ((current & 0x800) == (ksp & 0x800)))
		{
		  if (!jdb_disasm_addr_task(*(Address*)current,
					    ((current & 0x7ff) >= 0x7ec) 
					      ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case 'u':
	      if (jdb_disasm_addr_task && ((current & 0x800) == (ksp & 0x800)))
		{
		  Jdb::printf_statline("u[address=%08x task=%x] ",
					  *(Address*)current, t->id().task());
		  int c1 = getchar();
		  if ((c1 != KEY_RETURN) && (c1 != ' '))
		    {
		      Jdb::print_current_tid_statline();
		      putchar('u');
		      Jdb::execute_command("u", c1);
		      return NOTHING;
		    }

		  if (!jdb_disasm_addr_task(*(Address*)current, 
					    ((current & 0x7ff) >= 0x7ec) 
					      ? t->id().task() : 0, level+1))
		    return NOTHING;
		  redraw_screen = true;
		}
	      break;
	    case 'r': // ready-list
	      putchar(c);
	      switch (getchar())
		{
		case 'n':
		  if (t->ready_next)
		    {
		      L4_uid new_tid = (Thread::lookup(t->ready_next))->id();
		      if (!new_tid.is_invalid())
			{
			  tid = new_tid;
			  goto new_tcb;
			}
		    }
		  break;
		case 'p':
		  if (t->ready_prev)
		    {
		      L4_uid new_tid = (Thread::lookup(t->ready_prev))->id();
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
	      putchar(c);
	      switch (c=getchar()) 
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
	      if ((current & 0x800) == (ksp & 0x800))
		{
		  Mword value;
		  int c;

		  Jdb::cursor(addy+17, 9*addx+6);
		  printf("        ");
		  Jdb::printf_statline("edit <%08x> = %08x%s",
				        current, *(Mword*)current, 
					is_current_thread
					? "  (<Space>=edit registers)" : "");
		  Jdb::cursor(addy+17, 9*addx+6);
		  c = getchar();
		  if (c==KEY_ESC)
		    {
		      redraw = true;
		      break;
		    }
		  if (c != ' ' || !is_current_thread)
		    {
		      // edit memory
		      putchar(c);
		      Jdb::printf_statline("edit <%08x> = %08x", 
			  current, *(Address*)current);
		      Jdb::cursor(addy+17, 9*addx+6);
		      if (!Jdb_input::get_mword(&value, c, 16))
			{
			  Jdb::cursor(addy+17, 9*addx+6);
			  printf("%08x", *(Address*)current);
			  break;
			}
		      else
			*(Address*)current = value;
		    }
		  else
		    {
		      // edit registers
		      char reg = -1;
		      unsigned *reg_ptr=0, x=0, y=0;

		      Jdb::cursor(addy+17, 9*addx+6);
		      printf("%08x", *(Mword*)current);

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
	      default:
		args      = &threadid;
		fmt       = "%t\n";
		next_char = first_char;
		return Jdb_module::EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	}
      else
	show(threadid, 0);
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_tcb::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "t", "tcb", "%C",
	  "t[<threadid>]\tshow current/given thread control block\n"
	  "t{+|-}\tshow current thread control block at Jdb every entry",
	  &first_char),
    };
  return cs;
}

PUBLIC
int const
Jdb_tcb::num_cmds() const
{
  return 1;
}

static Jdb_tcb jdb_tcb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

int
jdb_show_tcb(L4_uid tid, int level)
{ return Jdb_tcb::show(tid, level); }

