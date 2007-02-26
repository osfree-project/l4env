IMPLEMENTATION:

#include "jdb_dbinfo.h"
#include "jdb_lines.h"
#include "jdb_symbol.h"
#include "jdb_tbuf.h"
#include "jdb_thread_names.h"
#include "thread.h"
#include "timer.h"

class Jdb_output_frame : public Jdb_entry_frame
{};

class Jdb_status_page_frame : public Jdb_entry_frame
{};

class Jdb_log_frame : public Jdb_entry_frame
{};

class Jdb_log_3val_frame : public Jdb_log_frame
{};

class Jdb_debug_frame : public Jdb_entry_frame
{};

class Jdb_symbols_frame : public Jdb_debug_frame
{};

class Jdb_lines_frame : public Jdb_debug_frame
{};

class Jdb_get_cputime_frame : public Jdb_entry_frame
{};

class Jdb_thread_name_frame : public Jdb_entry_frame
{};

//---------------------------------------------------------------------------
PUBLIC inline
Unsigned8*
Jdb_output_frame::str()
{ return (Unsigned8*)eax; }

PUBLIC inline
int
Jdb_output_frame::len()
{ return (unsigned)ebx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_status_page_frame::set(Address status_page)
{ eax = (Mword)status_page; }

//---------------------------------------------------------------------------
PUBLIC inline
Unsigned8*
Jdb_log_frame::str()
{ return (Unsigned8*)edx; }

PUBLIC inline
void
Jdb_log_frame::set_tb_entry(Tb_entry* tb_entry)
{ eax = (Mword)tb_entry; }

//---------------------------------------------------------------------------
PUBLIC inline
Mword
Jdb_log_3val_frame::val1()
{ return ecx; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val2()
{ return esi; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val3()
{ return edi; }

//---------------------------------------------------------------------------
PUBLIC inline
Task_num
Jdb_debug_frame::task()
{ return (Task_num)ebx; }

PUBLIC inline
Mword
Jdb_debug_frame::addr()
{ return eax; }

PUBLIC inline
Mword
Jdb_debug_frame::size()
{ return edx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_get_cputime_frame::invalidate()
{ ecx = 0xffffffff; }

//---------------------------------------------------------------------------
PUBLIC inline
const char *
Jdb_thread_name_frame::name()
{ return (const char*)eax; }

//---------------------------------------------------------------------------
IMPLEMENTATION[v2]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  eax = total_us;
  edx = total_us >> 32;
  ecx = prio;
  esi = next.raw(); 
  edi = next.raw() >> 32;
}

PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst()
{ return L4_uid((Unsigned64)edi << 32 | (Unsigned64)esi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst()
{ return L4_uid((Unsigned64)edi << 32 | (Unsigned64)esi); }

//---------------------------------------------------------------------------
IMPLEMENTATION[x0]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  eax = total_us;
  edx = total_us >> 32;
  ecx = prio;
  esi = next.raw();
}

PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst()
{ return L4_uid(esi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst()
{ return L4_uid(esi); }


//---------------------------------------------------------------------------
IMPLEMENTATION:

/**
 * Handle int3 extensions in the current thread's context. All functions
 * for which we don't need/want to switch to the debugging stack.
 * \return 0 if this function should be handled in the context of Jdb
 *         1 successfully handled
 */
PRIVATE static
int
Jdb::handle_int3_threadctx_generic(Trap_state *ts)
{
  Thread *t   = current_thread();
  Space *s    = t->space();
  Address ip = ts->ip();
  Address_type user;
  Unsigned8 *str, todo;
  int len;
  char c;

  entry_frame = reinterpret_cast<Jdb_entry_frame*>(ts);
  user = entry_frame->from_user();
  todo = s->peek((Unsigned8*)ip, user);

  switch (todo)
    {
    case 0xeb: // jmp == enter_kdebug()
      len = s->peek((Unsigned8*)(ip+1), user);
      str = (Unsigned8*)(ip + 2);

      if ((len > 0) && s->peek(str, user) == '*')
	{
          int i;

	  // skip '*'
	  len--; str++;

     	  if ((len > 0) && s->peek(str, user) == '#')
	    // special: enter_kdebug("*#...")
 	    return 0; // => Jdb

      	  if (s->peek(str, user) == '+')
	    {
	      // special: enter_kdebug("*+...") => extended log msg
	      // skip '+'
	      len--; str++;
	      Tb_entry_ke_reg *tb =
		static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
	      tb->set(t, ip-1, ts);
	      for (i=0; i<len; i++)
      		tb->set_buf(i, s->peek(str++, user));
	      tb->term_buf(len);
	    }
	  else
	    {
	      // special: enter_kdebug("*...") => log entry
	      // fill in entry
	      Tb_entry_ke *tb =
		static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
	      tb->set(t, ip-1);
	      for (i=0; i<len; i++)
		tb->set_buf(i, s->peek(str++, user));
	      tb->term_buf(len);
	    }
	  Jdb_tbuf::commit_entry();
	  break;
	}
      return 0; // => Jdb

    case 0x90: // nop == l4kd_display()
      if (          s->peek((Unsigned8*)(ip+1), user) != 0xeb /*jmp*/
	  || (len = s->peek((Unsigned8*)(ip+2), user)) <= 0)
	return 0; // => Jdb
      str = (Unsigned8*)(ip + 3);
      for (; len; len--)
	putchar(s->peek(str++, user));
      break;

    case 0x3c: // cmpb
	{
      todo = s->peek((Unsigned8*)(ip+1), user);
      Jdb_output_frame *regs = reinterpret_cast<Jdb_output_frame*>(ts);
      switch (todo)
	{
	case  0: // l4kd_outchar
	  putchar(regs->value() & 0xff);
	  break;
        case  1: // l4kd_outnstring
	  str = regs->str();
    	  len = regs->len();
	  for(; len > 0; len--)
	    putchar(s->peek(str++, user));
	  break;
	case  2: // l4kd_outstr
	  str = regs->str();
	  for (; (c=s->peek(str++, user)); )
      	    putchar(c);
	  break;
	case  5: // l4kd_outhex32 
	  printf("%08lx", regs->value() & 0xffffffff);
	  break;
	case  6: // l4kd_outhex20 
	  printf("%05lx", regs->value() & 0xfffff);
	  break;
	case  7: // l4kd_outhex16 
	  printf("%04lx", regs->value() & 0xffff);
	  break;
	case  8: // L4kd_outhex12
	  printf("%03lx", regs->value() & 0xfff);
	  break;
	case  9: // l4kd_outhex8 
	  printf("%02lx", regs->value() & 0xff);
	  break;
	case 11: // l4kd_outdec
	  printf("%ld", regs->value());
	  break;
	case 13: // l4kd_inchar
	  return 0; // => Jdb
	case 29:
	  switch (entry_frame->param())
	    {
	    case 0: // fiasco_tbuf_get_status()
		{
		  Jdb_status_page_frame *regs =
		    reinterpret_cast<Jdb_status_page_frame*>(ts);
		  regs->set(Mem_layout::Tbuf_status_page);
		}
	      break;
	    case 1: // fiasco_tbuf_log()
		{
		  // interrupts are disabled in handle_slow_trap()
		  Jdb_log_frame *regs = reinterpret_cast<Jdb_log_frame*>(ts);
		  Tb_entry_ke *tb =
		    static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
		  str = regs->str();
		  tb->set(t, ip-1);
		  for (len=0; (c = s->peek(str++, user)); len++)
		    tb->set_buf(len, c);
		  tb->term_buf(len);
		  regs->set_tb_entry(tb);
		  Jdb_tbuf::commit_entry();
		}
	      break;
	    case 2: // fiasco_tbuf_clear()
	      Jdb_tbuf::clear_tbuf();
	      break;
	    case 3: // fiasco_tbuf_dump()
	      return 0; // => Jdb
	    case 4: // fiasco_tbuf_log_3val()
		{
		  // interrupts are disabled in handle_slow_trap()
		  Jdb_log_3val_frame *regs = 
		    reinterpret_cast<Jdb_log_3val_frame*>(ts);
		  Tb_entry_ke_reg *tb =
		    static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
		  str = regs->str();
		  tb->set(t, ip-1, regs->val1(), regs->val2(), regs->val3());
		  for (len=0; (c = s->peek(str++, user)); len++)
		    tb->set_buf(len, c);
		  tb->term_buf(len);
		  regs->set_tb_entry(tb);
		  Jdb_tbuf::commit_entry();
		}
	      break;
	    case 5: // fiasco_tbuf_get_status_phys()
		{
		  Jdb_status_page_frame *regs =
		    reinterpret_cast<Jdb_status_page_frame*>(ts);
		  regs->set(s->virt_to_phys(Mem_layout::Tbuf_status_page));
		}
	      break;
	    case 6: // fiasco_timer_disable
	      Timer::disable();
	      break;
	    case 7: // fiasco_timer_enable
	      Timer::enable();
	      break;
	    }
	  break;
	case 30:
	  switch (ts->ecx)
	    {
	    case 1: // fiasco_register_symbols
		{
		  Jdb_symbols_frame *regs =
		    reinterpret_cast<Jdb_symbols_frame*>(ts);
		  Jdb_dbinfo::set(Jdb_symbol::lookup(regs->task()),
				  regs->addr(), regs->size());
		}
	      break;
	    case 2: // fiasco_register_lines
		{
		  Jdb_lines_frame *regs =
		    reinterpret_cast<Jdb_lines_frame*>(ts);
		  Jdb_dbinfo::set(Jdb_lines::lookup(regs->task()),
				  regs->addr(), regs->size());
		}
	      break;
	    case 3: // fiasco_register_thread_name
		{
		  Jdb_thread_name_frame *regs =
		    reinterpret_cast<Jdb_thread_name_frame*>(ts);
		  Jdb_thread_names::register_thread(regs->dst(), regs->name());
		}
	      break;
	    case 4: // fiasco_get_cputime
		{
		  Jdb_get_cputime_frame *regs =
		    reinterpret_cast<Jdb_get_cputime_frame*>(ts);
		  if (!(t = Thread::lookup(regs->dst())))
		    {
		      regs->invalidate();
		      break;
		    }
		  if (t == get_thread())	// Update if current thread
		    t->update_consumed_time();
		  regs->set(t->present_next->id(), t->consumed_time(),
		      t->sched()->prio());
		}
	      break;
	    }
	  break;
	default: // ko
	  if (todo < ' ')
	    return 0; // => Jdb

	  putchar(todo);
	  break;
	}
      break;
	}

    default:
      return 0; // => Jdb
    }

  return 1;
}
