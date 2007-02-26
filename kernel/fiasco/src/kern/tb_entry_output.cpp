IMPLEMENTATION:

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "config.h"
#include "jdb_symbol.h"
#include "jdb_tbuf_output.h"
#include "static_init.h"
#include "tb_entry.h"
#include "thread.h"

static __attribute__((format(printf, 3, 4)))
void
my_snprintf(char *&buf, int &maxlen, const char *format, ...)
{
  int len;
  va_list list;

  if (maxlen<=0)
    return;

  va_start(list, format);
  len = vsnprintf(buf, maxlen, format, list);
  va_end(list);

  if (len<0 || len>=maxlen)
    len = maxlen-1;

  buf    += len;
  maxlen -= len;
}

// timeout => x.x{
static
void
format_timeout(Mword us, char *&buf, int &maxlen)
{
  if (us >= 1000000000)		// =>100s
    my_snprintf(buf, maxlen, ">99s");
  else if (us >= 10000000)	// >=10s
    my_snprintf(buf, maxlen, "%lds", us/1000000);
  else if (us >= 1000000)	// >=1s
    my_snprintf(buf, maxlen, "%ld.%lds", us/1000000, (us%1000000)/100000);
  else if (us >= 10000)		// 10ms
    my_snprintf(buf, maxlen, "%ldm", us/1000);
  else if (us >= 1000)		// 1ms
    my_snprintf(buf, maxlen, "%ld.%ldm", us/1000, (us%1000)/100);
  else
    my_snprintf(buf, maxlen, "%ld%c", us, Config::char_micro);
}

IMPLEMENTATION:

// ipc / irq / shortcut success
static
unsigned
formatter_ipc(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ipc *e = static_cast<Tb_entry_ipc*>(tb);

  if (   e->snd_desc().is_register_ipc()
      && e->rcv_desc().is_register_ipc()
      && e->dst().is_irq())
    {
      // irq raised
      my_snprintf(buf, maxlen, "IRQ %02lx %sraised at %s @ " L4_PTR_FMT,
	  e->dst().irq(),
          e->dst().irq() == Config::scheduler_irq_vector ? "[TIMER] " : "",
	  tidstr, e->ip());
    }
  else
    {
      // ipc operation
      const char *m;

      my_snprintf(buf, maxlen, "%s: ", (e->type()==Tbuf_ipc) ? "ipc" : "sc ");

      if (e->snd_desc().has_snd())
	m = (e->rcv_desc().has_receive())
		 ? (e->rcv_desc().open_wait()) ? "repl->" : "call->"
		 : "send->";
      else
	m = (e->rcv_desc().has_receive())
		 ? (e->rcv_desc().open_wait()) ? "waits"  : "recv from"
	     	 : "(no ipc) ";

      my_snprintf(buf, maxlen, "%s%6s %s", 
	  e->dst().next_period() ? "[NP] " : "", tidstr, m);

      // print destination id
      if (!e->rcv_desc().open_wait() || e->snd_desc().has_snd())
	{
	  if (e->dst().is_nil())
	    my_snprintf(buf, maxlen, " -");
	  else if (e->dst().is_irq())
	    my_snprintf(buf, maxlen, " irq %02lx", e->dst().irq());
	  else
	    my_snprintf(buf, maxlen, " %x.%02x",
			e->dst().d_task(), e->dst().d_thread());
	}

      // print mwords if have send part
      if (e->snd_desc().has_snd())
	{
	  if (e->snd_desc().map())
	    my_snprintf(buf, maxlen, e->dword(1) & 1 ? " grant" : " map");

	  if (!e->dst().next_period())
	    my_snprintf(buf, maxlen, " ("L4_PTR_FMT","L4_PTR_FMT")", 
		        e->dword(0), e->dword(1));
	}

      my_snprintf(buf, maxlen, " TO=");

      L4_timeout to = e->timeout();
      if (e->snd_desc().has_snd())
	{
	  if (e->dst().abs_snd_timeout())
	    {
	      // absolute send timeout
	      Unsigned64 end = to.snd_microsecs_abs (e->kclock(), 
						     e->dst().abs_snd_clock());
	      format_timeout((Mword)(end > e->kclock() ? end-e->kclock() : 0),
			     buf, maxlen);
	    }
	  else
	    {
	      // relative send timeout
	      if (to.snd_exp() == 0)
		my_snprintf(buf, maxlen, "INF");
	      else if (to.snd_man() == 0)
		my_snprintf(buf, maxlen, "0");
	      else
		format_timeout((Mword)to.snd_microsecs_rel(0), buf, maxlen);
	    }
	}
      if (e->snd_desc().has_snd() && e->rcv_desc().has_receive())
	my_snprintf(buf, maxlen, "/");
      if (e->rcv_desc().has_receive())
	{
	  if (e->dst().abs_rcv_timeout())
	    {
	      // absolute receive timeout
	      Unsigned64 end = to.rcv_microsecs_abs (e->kclock(),
						     e->dst().abs_rcv_clock());
	      format_timeout((Mword)(end > e->kclock() ? end-e->kclock() : 0),
			     buf, maxlen);
	    }
	  else
	    {
	      // relative receive timeout
	      if (to.rcv_exp() == 0)
		my_snprintf(buf, maxlen, "INF");
	      else if (to.rcv_man() == 0)
		my_snprintf(buf, maxlen, "0");
	      else
		format_timeout((Mword)to.rcv_microsecs_rel(0), buf, maxlen);
	    }
	}

      if (e->dst().next_period())
	{
    	  my_snprintf(buf, maxlen, " left:%lld",
	      (Unsigned64)e->dword(0) +
	      ((Unsigned64)e->dword(1) << 32));
	}
    }

  return maxlen;
}

// short-cut ipc operation failed
static
unsigned
formatter_sc_failed(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ipc_sfl *e = static_cast<Tb_entry_ipc_sfl*>(tb);
  const char *m;
  
  my_snprintf(buf, maxlen, "!sc: %6s ", tidstr);
     
  if (e->snd_desc().has_snd())
    m = (e->rcv_desc().has_receive())
      ? (e->rcv_desc().open_wait()) ? "repl->" : "call->"
      : "snd->";
  else
    m = (e->rcv_desc().open_wait()) ? "waits" : "recv from";

  my_snprintf(buf, maxlen, "%s", m);

  // print dst id
  if (!(e->rcv_desc().open_wait()) || (e->snd_desc().has_snd()))
    {
      if (e->dst().is_nil())
	my_snprintf(buf, maxlen, " -");
      else if (e->dst().is_irq())
	my_snprintf(buf, maxlen, " irq %02lx", e->dst().irq());
      else
      	my_snprintf(buf, maxlen, " %x.%02x", 
	    e->dst().d_task(), e->dst().d_thread());
    }

  my_snprintf(buf, maxlen, " @ "L4_PTR_FMT" (rsn: ", e->ip());

  m = "unknown";

  if (e->dst_ok())
    m = "!dest sender_ok";
  else if (e->dst_lck())
    m = "dest locked";
  else if (!e->snd_desc().has_snd())
    m = "no send";
  else if (e->snd_desc().is_long_ipc())
    m = "long send";
  else if (e->dst().is_invalid())
    m = "dest invalid";
  else if (e->dst().is_nil())
    m = "dest nil";
  else if (e->dst().next_period())
    m = "next period";
  else if (e->rcv_desc().has_receive())
    {
      if (!e->rcv_desc().is_register_ipc())
	m = "long recv";
      else if (Config::deceit_bit_disables_switch && e->snd_desc().deceite())
	m = "don't switch on recv";
      else if (e->timeout().rcv_exp() && e->timeout().rcv_man())
	m = "to != (0, inf)";
      else if (e->rcv_desc().open_wait())
	{
	  if (e->is_irq())
	    m = "irq attached";
	  else if (e->snd_lst())
	    m = "sender queued";
	}
    }

  my_snprintf(buf, maxlen, "%s)", m);

  return maxlen;
}

// result of ipc
static
unsigned
formatter_ipc_res(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ipc_res *e = static_cast<Tb_entry_ipc_res*>(tb);
  Mword error = e->result().error();
  const char *m;

  if (e->have_snd())
    m = (e->rcv_desc().has_receive())
		? (e->rcv_desc().open_wait()) ? "rply" : "call"
		: "send";
  else
    m = (e->rcv_desc().open_wait()) ? "wait" : "recv";

  my_snprintf(buf, maxlen, "     %s%6s %s dope="L4_PTR_FMT" (%s%s) ",
			    e->is_np() ? "[np] " : "",
			    tidstr, m, e->result().raw(),
			    error ? "L4_IPC_" : "", e->result().str_error());

  if (e->rcv_desc().has_receive())
    {
      if (   !e->rcv_desc().open_wait()
	  || (   e->result().error() != Ipc_err::Retimeout
	      && e->result().error() != Ipc_err::Recanceled))
	if (e->rcv_src().is_irq())
	  my_snprintf(buf, maxlen, " irq %02lx", e->rcv_src().irq());
	else
	  my_snprintf(buf, maxlen, "%x.%02x",
				 e->rcv_src().d_task(), e->rcv_src().d_thread());
      if (!error)
	{
     	  if (e->result().fpage_received())
	    my_snprintf(buf, maxlen, "%s", 
		e->dword(1) & 1 ? " grant" : " map");
	  my_snprintf(buf, maxlen, 
	      " ("L4_PTR_FMT","L4_PTR_FMT")", e->dword(0), e->dword(1));
	}
    }

  return maxlen;
}

IMPLEMENTATION:

// pagefault
static
unsigned
formatter_pf(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_pf *e = static_cast<Tb_entry_pf*>(tb);
  char ip_buf[16];

  snprintf(ip_buf, sizeof(ip_buf), L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "pf:  %6s pfa="L4_PTR_FMT" ip=%s (%c%c) spc=%02x",
      tidstr, e->pfa(), e->ip() ? ip_buf : "unknown",
      e->error() & 2 ? (e->error() & 4 ? 'w' : 'W')
		     : (e->error() & 4 ? 'r' : 'R'),
      e->error() & 1 ? 'p' : '-',
      (unsigned)e->space()->id());

  return maxlen;
}

// pagefault result
static
unsigned
formatter_pf_res(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_pf_res *e = static_cast<Tb_entry_pf_res*>(tb);

  // e->ret contains only an error code
  // e->err contains only up to 3 dwords
  my_snprintf(buf, maxlen, "     %6s pfa="L4_PTR_FMT" dope=%02lx (%s%s) "
		"err=%04lx (%s%s)",
	      tidstr, e->pfa(),
	      e->ret().raw(), e->ret().error() ? "L4_IPC_" : "",
	      e->ret().str_error(),
	      e->err().raw(), e->err().error() ? "L4_IPC_" : "",
	      e->err().str_error());

  return maxlen;
}

// kernel event (enter_kdebug("*..."))
static
unsigned
formatter_ke(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ke *e = static_cast<Tb_entry_ke*>(tb);
  char ip_buf[16];

  snprintf(ip_buf, sizeof(ip_buf), " @ "L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "ke:  %6s \"%s\"%s",
      tidstr, e->msg(), e->ip() ? ip_buf : "");

  return maxlen;
}

// kernel event (enter_kdebug_verb("*+...", val1, val2, val3))
static
unsigned
formatter_ke_reg(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ke_reg *e = static_cast<Tb_entry_ke_reg*>(tb);

  char ip_buf[16];
  snprintf(ip_buf, sizeof(ip_buf), " @ "L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "ke:  %6s \"%s\" "
      L4_PTR_FMT" "L4_PTR_FMT" "L4_PTR_FMT"%s",
      tidstr, e->msg(), e->val1(), e->val2(), e->val3(), e->ip() ? ip_buf : "");

  return maxlen;
}

// breakpoint
static
unsigned
formatter_bp(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_bp *e = static_cast<Tb_entry_bp*>(tb);

  my_snprintf(buf, maxlen, "b%c:  %6s @ "L4_PTR_FMT" ",
      "iwpa"[e->mode() & 3], tidstr, e->ip());
  switch (e->mode() & 3)
    {
    case 1:
    case 3:
      switch (e->len())
	{
     	case 1:
	  my_snprintf(buf, maxlen, "["L4_PTR_FMT"]=%02lx", 
	              e->addr(), e->value());
	  break;
	case 2:
	  my_snprintf(buf, maxlen, "["L4_PTR_FMT"]=%04lx", 
	      	      e->addr(), e->value());
	  break;
	case 4:
	  my_snprintf(buf, maxlen, "["L4_PTR_FMT"]="L4_PTR_FMT, 
	      	      e->addr(), e->value());
	  break;
	}
      break;
    case 2:
      my_snprintf(buf, maxlen, "["L4_PTR_FMT"]", e->addr());
      break;
    }

  return maxlen;
}

// context switch
static
unsigned
formatter_ctx_switch(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  char symstr[24], spcstr[16] = "";
  Tb_entry_ctx_sw *e = static_cast<Tb_entry_ctx_sw*>(tb);
  unsigned  spc      = (unsigned)((Space*)(e->from_space()))->id();
  Context   *sctx    = e->from_sched()->owner();
  Global_id sctxid   = Thread::lookup(sctx)->id();
  Global_id src      = Thread::lookup(e->ctx())->id();
  Global_id dst      = Thread::lookup(e->dst())->id();
  Global_id dst_orig = Thread::lookup(e->dst_orig())->id();
  Address addr       = e->kernel_ip();

  if (!Jdb_symbol::match_addr_to_symbol_fuzzy(&addr, 0 /*kernel*/,
					      symstr, sizeof(symstr)))
    snprintf(symstr, sizeof(symstr), L4_PTR_FMT, e->kernel_ip());

  if (Config::Small_spaces)
    {
      if (spc != src.d_task())
	snprintf(spcstr, sizeof(spcstr), " (%2x)", spc);
      else
	strcpy(spcstr, "     ");
    }

  my_snprintf(buf, maxlen, "     %6s%s '%02lx", tidstr, spcstr, e->from_prio());
  if (sctx != e->ctx())
    my_snprintf(buf, maxlen, "(%x.%02x)", sctxid.d_task(), sctxid.d_thread());

  my_snprintf(buf, maxlen, " ==> %3x.%02x ", dst.d_task(), dst.d_thread());

  if (dst != dst_orig || e->lock_cnt())
    my_snprintf(buf, maxlen, "(");

  if (dst != dst_orig)
    my_snprintf(buf, maxlen, "want %x.%02x",
	dst_orig.d_task(), dst_orig.d_thread());

  if (dst != dst_orig && e->lock_cnt())
    my_snprintf(buf, maxlen, " ");

  if (e->lock_cnt())
    my_snprintf(buf, maxlen, "lck %ld", e->lock_cnt());

  if (dst != dst_orig || e->lock_cnt())
    my_snprintf(buf, maxlen, ") ");

  my_snprintf(buf, maxlen, " krnl %s", symstr);

  return maxlen;
}

// unmap system call
static
unsigned
formatter_unmap(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_unmap *e = static_cast<Tb_entry_unmap*>(tb);

  my_snprintf(buf, maxlen, 
      "ump: %6s: @ "L4_PTR_FMT" size %dkB, %s %s, @ "L4_PTR_FMT,
      tidstr,
      e->fpage().page(), 1 << (e->fpage().size() - 10),
      e->mask() & 2 ? "flush" : "remap",
      e->mask() & 0x80000000 ? "all" : "other", e->ip());

  return maxlen;
}

// thread_ex_regs system call
static
unsigned
formatter_ex_regs(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_ex_regs *e = static_cast<Tb_entry_ex_regs*>(tb);

  my_snprintf(buf, maxlen, "exr: %6s @ "L4_PTR_FMT": ", tidstr, e->ip());
  if (e->task())
    my_snprintf(buf, maxlen, "%lx.", e->task());
  my_snprintf(buf, maxlen, "%02lx ", e->lthread());
  if (e->failed())
    my_snprintf(buf, maxlen, " => FAILED");
  else
    {
      if (e->new_ip() != ~0UL)
	my_snprintf(buf, maxlen, "eip "L4_PTR_FMT"=>"L4_PTR_FMT" ", 
		    e->old_ip(), e->new_ip());
      if (e->new_sp() != ~0UL)
	my_snprintf(buf, maxlen, "esp="L4_PTR_FMT, e->new_sp());
    }

  return maxlen;
}

// trap
static
unsigned
formatter_trap(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_trap *e = static_cast<Tb_entry_trap*>(tb);

  if (e->trapno() & 0x80)
    my_snprintf(buf, maxlen, "#%02x: %6s @ "L4_PTR_FMT, 
		e->trapno() & 0x7f, tidstr, e->ip());
  else
    my_snprintf(buf, maxlen, 
	        e->trapno() == 14
		  ? "#%02x: %6s err=%04x @ "L4_PTR_FMT
		    " cs=%04x esp="L4_PTR_FMT" cr2="L4_PTR_FMT
		  : "#%02x: %6s err=%04x @ "L4_PTR_FMT
		    " cs=%04x esp="L4_PTR_FMT" fl="L4_PTR_FMT,
	        e->trapno(), tidstr, e->errno(), e->ip(), e->cs(), e->sp(), 
		e->trapno() == 14 ? e->cr2() : e->eax());

  return maxlen;
}

// sched
static
unsigned
formatter_sched(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_sched *e = static_cast<Tb_entry_sched*>(tb);
  Thread *t = Thread::lookup (e->owner());

  my_snprintf (buf, maxlen, 
            "%6s (ts %s) owner:%2x.%02x id:%2x, prio:%2x, left:%6ld/%-6lu",
               tidstr,
               e->mode() == 0 ? "save" :
               e->mode() == 1 ? "load" :
               e->mode() == 2 ? "invl" : "????",
               t->id().d_task(), t->id().d_thread(),
               e->id(), e->prio(), e->left(), e->quantum());

  return maxlen;
}

// preemption
static
unsigned
formatter_preemption(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_preemption *e = static_cast<Tb_entry_preemption*>(tb);
  Thread *t = Thread::lookup (e->preempter());

  my_snprintf (buf, maxlen,
	     "pre: %6s sent to %x.%02x", 
	     tidstr, t->id().d_task(), t->id().d_thread());

  return maxlen;
}


// lipc
static
unsigned
formatter_lipc(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_lipc *e = static_cast<Tb_entry_lipc*>(tb);
  
  switch(e->type())
    {
    case 1:
      my_snprintf (buf, maxlen,
		   "lipc: rollback kernel: %s utcbptr: "L4_PTR_FMT,
		   tidstr, e->c_utcb_ptr());
      break;
    case 2:
      my_snprintf (buf, maxlen,
		   "lipc: rollforw kernel: %s %2x.%02x -> %2x.%02x",
		   tidstr,
		   e->old_thread().d_task(), e->old_thread().d_thread(),
		   e->new_thread().d_task(), e->new_thread().d_thread());
      break;
    case 3:
      my_snprintf (buf, maxlen,
		   "lipc: stack cp kernel: %s -> %2x.%02x",
		   tidstr,
		   e->new_thread().d_task(), e->new_thread().d_thread());
      break;
    case 4:
      my_snprintf (buf, maxlen,
		   "lipc: iret st. kernel: %s old: %2x.%02x",
		   tidstr,
		   e->old_thread().d_task(), e->old_thread().d_thread());
      break;
    }
  
  return maxlen;
}


// jean1
static
unsigned
formatter_jean1(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_jean1 *e = static_cast<Tb_entry_jean1*>(tb);
  Thread *t1 = Thread::lookup (e->sched_owner1());
  Thread *t2 = Thread::lookup (e->sched_owner2());

  my_snprintf (buf, maxlen,
	       "jean1: %s sched1=%x.%02x sched2=%x.%02x",
	       tidstr,
	       t1->id().d_task(), t1->id().d_thread(),
	       t2->id().d_task(), t2->id().d_thread());

  return maxlen;
}


// task_new
static
unsigned
formatter_task_new(Tb_entry *tb, const char *tidstr, char *buf, int maxlen)
{
  Tb_entry_task_new *e = static_cast<Tb_entry_task_new*>(tb);

  my_snprintf (buf, maxlen, "tsk: %6s -> %x.%02x pager=",
	       tidstr, e->task().d_task(), e->task().d_thread());

  if (e->pager().is_nil())
    {
      L4_uid chief(e->mcp_or_chief());
      my_snprintf(buf, maxlen, "NIL (inactive) new owner=%x.%02x",
	         chief.d_task(), chief.d_thread());
    }
  else
    my_snprintf(buf, maxlen,
		"%x.%02x eip="L4_PTR_FMT" esp="L4_PTR_FMT" mcp=%02lx",
	        e->pager().d_task(), e->pager().d_thread(),
	   	e->new_ip(), e->new_sp(), e->mcp_or_chief());

  return maxlen;
}

STATIC_INITIALIZER(init_formatters);

// register all available format functions
static FIASCO_INIT
void
init_formatters()
{
  Jdb_tbuf_output::register_ff(Tbuf_pf, formatter_pf);
  Jdb_tbuf_output::register_ff(Tbuf_pf_res, formatter_pf_res);
  Jdb_tbuf_output::register_ff(Tbuf_ipc, formatter_ipc);
  Jdb_tbuf_output::register_ff(Tbuf_ipc_res, formatter_ipc_res);
  Jdb_tbuf_output::register_ff(Tbuf_ke, formatter_ke);
  Jdb_tbuf_output::register_ff(Tbuf_ke_reg, formatter_ke_reg);
  Jdb_tbuf_output::register_ff(Tbuf_unmap, formatter_unmap);
  Jdb_tbuf_output::register_ff(Tbuf_shortcut_failed, formatter_sc_failed);
  Jdb_tbuf_output::register_ff(Tbuf_shortcut_succeeded, formatter_ipc);
  Jdb_tbuf_output::register_ff(Tbuf_context_switch, formatter_ctx_switch);
  Jdb_tbuf_output::register_ff(Tbuf_exregs, formatter_ex_regs);
  Jdb_tbuf_output::register_ff(Tbuf_breakpoint, formatter_bp);
  Jdb_tbuf_output::register_ff(Tbuf_trap, formatter_trap);
  Jdb_tbuf_output::register_ff(Tbuf_sched, formatter_sched);
  Jdb_tbuf_output::register_ff(Tbuf_preemption, formatter_preemption);
  Jdb_tbuf_output::register_ff(Tbuf_lipc, formatter_lipc);
  Jdb_tbuf_output::register_ff(Tbuf_jean1, formatter_jean1);
  Jdb_tbuf_output::register_ff(Tbuf_task_new, formatter_task_new);
}
