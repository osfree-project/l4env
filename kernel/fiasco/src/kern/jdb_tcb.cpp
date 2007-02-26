IMPLEMENTATION:

#include <cstdio>
#include <simpleio.h>

#include "jdb.h"
#include "jdb_module.h"
#include "static_init.h"
#include "thread.h"
#include "threadid.h"
#include "types.h"


//===================
// Std JDB modules
//===================

/**
 * @brief 'TCB' module.
 * 
 * This module handles the 't' command that
 * provides TCB view.
 */
class Jdb_tcb
  : public Jdb_module
{
public:
private:
  static void show_tcb( Thread *t );
  struct {
    unsigned task;
    unsigned thread;
  } id;
};

static Jdb_tcb jdb_tcb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


IMPLEMENT
void Jdb_tcb::show_tcb( Thread *t )
{
  putchar('\n');
  if (t && !t->state())
    {
      puts(" Invalid thread");
      return;
    }

  printf("thread: @%p (", t); t->id().print(); puts(")");
  printf("state: %03x ", t->state());
  t->print_state_long();
  putstr("\n\nwait for: "); t->print_partner();
  putstr("   polling: "); t->print_send_partner();

  putchar('\n');

  for(Unsigned32 *a = (Unsigned32*)t + Config::thread_block_size/sizeof(Unsigned32) - 10*8; 
      a < ((Unsigned32*)t + Config::thread_block_size/sizeof(Unsigned32));
      ++a) 
    {
      if((Address)a%32==0)
        printf("%03x: ",(Address)a & 0x0fff);

      printf("%08x%c", *a, (Address)a%32==28 ? '\n' : ' ');
    }
  putchar('\n');

}

#if 0
IMPLEMENT 
bool Jdb_tcb::xshow_tcb(/*trap_state *ts,*/ L4_uid tid, int level)
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

  Threadid t_id(&tid);
  Thread *t = current_active;
  bool redraw_screen = true, is_current_thread = true;


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

  const Mword lines    = 8;
  const Mword max_absy = Config::thread_block_size/32 - lines;
  Mword ksp            = (Mword)t->kernel_sp;
  Mword tcb            = ksp & ~(Config::thread_block_size-1);
  Mword absy           = (ksp - tcb) / 32;
  Mword addy           = 0;
  Mword addx           = (ksp & 31) / 4;
  Mword *current;

  if (level==0)
    {
      fancy_clear_screen();
      redraw_screen = false;
    }
  
 whole_screen:
  
  if (redraw_screen)
    {
      home();
      for(int i=0; i<24; i++)
	{
	  clear_to_eol();
	  putchar('\n');
	}
      home();
      redraw_screen = false;
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
	 t->sched()->prio(), t->mcp());

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

  // dump the stack from ksp bottom right to tcb_top
  Mword p = tcb + absy*32;
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
	      unsigned va = kdir_lookup(p, t->_id.task());
  	      if (va != 0xffffffff)
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

  printf_statline("t%73s", "e=edit u=disasm r|p+p|n=ready/present next/prev");

  for (bool redraw=false; ; )
    {
      current = (Mword *)(tcb + absy*32 + addy*32 + addx*4);
      if (((Mword)current & 0x800) == (ksp & 0x800))
	{
	  cursor(addy+16, 9*addx+5);
	  printf("\033[33;1m%08x\033[m", *current);
	}
      cursor(addy+16, 9*addx+5);
      int c=getchar();
      if (((Mword)current & 0x800) == (ksp & 0x800))
	{
	  cursor(addy+16, 9*addx+5);
	  printf("\033[%sm%08x\033[m",
		 (((Mword)current & 0x7ff) >= 0x7ec) ? "36;1" : "",
	         *current);
	}
      cursor(Jdb_screen::height(), LOGO);

      if ((c == KEY_CURSOR_HOME) && (level > 0))
	return true;

      if (!std_cursor_key(c, lines, max_absy, &absy, &addy, &addx, &redraw))
	{
	  switch (c)
	    {
	    case KEY_RETURN:
	      if (((unsigned)current & 0x800) == (ksp & 0x800))
		{
		  if (!dump_address_space(ts, D_MODE, (Address)*current,
					  t->_id.task(), level+1))
		    return false;
		  redraw_screen = true;
		}
	      break;
	    case ' ':
	      // if not linked, do nothing
	      if ((HAVE_DISASM)
		  && (((unsigned)current & 0x800) == (ksp & 0x800)))
		{
		  if (!disasm_address_space(*current, t->_id.task(), level+1))
		    return false;
		  redraw_screen = true;
		}
	      break;
	    case 'u':
	      if ((HAVE_DISASM)
		  && (((unsigned)current & 0x800) == (ksp & 0x800)))
		{
		  printf_statline("u[address=%08x task=%x] ", 
				  (Address)*current, t->_id.task());
		  int c1 = getchar();
		  if ((c1 != KEY_RETURN) && (c1 != ' '))
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

		  if (!disasm_address_space((Address)*current, t->_id.task(), 
					    level+1))
		    return false;
		  redraw_screen = true;
		}
	      break;
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
	    case 'e': // edit memory/registers
	      if (((unsigned)current & 0x800) == (ksp & 0x800))
		{
		  Mword value;
		  int c;

		  cursor(addy+16, 9*addx+5);
		  printf("        ");
		  printf_statline("edit <%08x> = %08x%s",
				  current, *current, is_current_thread
				  ? "  (<Space>=edit registers)" : "");
		  cursor(addy+16, 9*addx+5);
		  c = getchar();
		  if (c==KEY_ESC)
		    break;
		  if (c != ' ' || !is_current_thread)
		    {
		      // edit memory
		      putchar(c);
		      printf_statline("edit <%08x> = %08x", current, *current);
		      cursor(addy+16, 9*addx+5);
		      if (!x_get_32(&value, 0, c))
			{
			  cursor(addy+16, 9*addx+5);
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

		      cursor(addy+16, 9*addx+5);
		      printf("%08x", *current);

		      printf_statline("edit register "
				      "e{ax|bx|cx|dx|si|di|sp|bp|ip|fl}: ");
		      cursor(Jdb_screen::height(), 53);
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
		      printf_statline("edit %s = %08x", 
				       reg_names[reg-1], *reg_ptr);
		      cursor(y, x);
		      if (x_get_32(&value, 0))
			*reg_ptr = value;
		      redraw_screen = true;
		      break;
		    }
		}
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
      if (redraw_screen)
	goto whole_screen;
      if (redraw)
	goto dump_stack;
    }

} 
#endif

PUBLIC Jdb_module::Action_code
Jdb_tcb::action( int cmd, void *&, char const *&, int & )
{
 
  if (cmd>1)
    return NOTHING;

  Thread *t = (Thread*)Jdb::current_context();
  Threadid tid(t->id());

  switch(cmd)
    {
    case 1:
      tid = Threadid(L4_uid(id.task,id.thread));
    case 0:
      show_tcb(tid.lookup());
      break;
    }

  return NOTHING;
}

PUBLIC
int const Jdb_tcb::num_cmds() const
{ 
  return 2;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_tcb::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "t", "tcb", "", 
	   "t\tshow tcb", const_cast<void*>((void*)&id) ),
      Cmd( 1, "T", "Tcb", " id: %x.%x", 
	   "T{id}\tshow tcb", const_cast<void*>((void*)&id) )
    };

  return cs;
}

