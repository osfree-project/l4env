IMPLEMENTATION:

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

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

  if (len<0 || len>maxlen)
    len = maxlen;

  buf    += len;
  maxlen -= len;
}

// ipc / irq / shortcut success
static
unsigned
formatter_ipc(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ipc *e = static_cast<Tb_entry_ipc*>(tb);

  my_snprintf(buf, maxlen, "%s: ", (e->type() == TBUF_IPC) ? "ipc" : "sc ");

  if (   e->snd_desc().is_register_ipc()
      && e->rcv_desc().is_register_ipc()
      && e->dest().is_irq())
    {
      // irq raised
      my_snprintf(buf, maxlen, "irq %02x raised at %x.%02x",
				 e->dest().irq(), tid.task(), tid.lthread());
    }
  else
    {
      // ipc operation
      const char *m;

      if (e->snd_desc().has_send())
	m = (e->rcv_desc().has_receive())
		 ? (e->rcv_desc().open_wait()) ? "repl->" : "call->"
		 : "send->";
      else
	m = (e->rcv_desc().has_receive())
		 ? (e->rcv_desc().open_wait()) ? "waits"  : "recv from"
	     	 : "(no ipc) ";

      my_snprintf(buf, maxlen, "%3x.%02x %s", 
				  tid.task(), tid.lthread(), m);

      // print destination id
      if (!e->rcv_desc().open_wait() || e->snd_desc().has_send())
	{
	  if (e->dest().is_nil())
	    my_snprintf(buf, maxlen, " -");
	  else if (e->dest().is_irq())
	    my_snprintf(buf, maxlen, " irq %02x", e->dest().irq());
	  else
	    my_snprintf(buf, maxlen, " %x.%02x",
					e->dest().task(), e->dest().lthread());
	}

      // print mwords if have send part
      if (e->snd_desc().has_send())
	{
	  if (e->snd_desc().map())
	    my_snprintf(buf, maxlen, e->dword(1) & 1 ? " grant" : " map");

#ifdef CONFIG_ABI_X0
	  my_snprintf(buf, maxlen, " (%08x,%08x,%08x)", 
	      e->dword(0), e->dword(1), e->dword(2));
#else
	  my_snprintf(buf, maxlen, " (%08x,%08x)",
	      e->dword(0), e->dword(1));
#endif
	}
    }

  my_snprintf(buf, maxlen, " @%08x", e->eip());

  return maxlen;
}

// short-cut ipc operation failed
static
unsigned
formatter_sc_failed(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ipc_sfl *e = static_cast<Tb_entry_ipc_sfl*>(tb);
  const char *m;
  
  my_snprintf(buf, maxlen, "!sc: %3x.%02x ", tid.task(), tid.lthread());
     
  if (e->snd_desc().has_send())
    m = (e->rcv_desc().has_receive())
      ? (e->rcv_desc().open_wait()) ? "replies->" : "-calls-->"
      : "-sends-->";
  else
    m = (e->rcv_desc().open_wait()) ? "waits" : "recv from";

  my_snprintf(buf, maxlen, "%s", m);

  // print dst id
  if (!(e->rcv_desc().open_wait()) || (e->snd_desc().has_send()))
    {
      if (e->dest().is_nil())
	my_snprintf(buf, maxlen, " -");
      else if (e->dest().is_irq())
	my_snprintf(buf, maxlen, " irq %02x", e->dest().irq());
      else
      	my_snprintf(buf, maxlen, " %x.%02x", 
	    e->dest().task(), e->dest().lthread());
    }

  my_snprintf(buf, maxlen, " @%08x (rsn: ", e->eip());

  m = "unknown";

  if (e->dst_ok())
    m = "!dest sender_ok";
  else if (e->dst_lck())
    m = "dest locked";
  else if (!e->snd_desc().has_send())
    m = "no send";
  else if (e->snd_desc().is_long_ipc())
    m = "long send";
  else if (!Threadid(e->dest()).is_valid())
    m = "dest invalid";
  else if (e->dest().is_nil())
    m = "dest nil";
  else if (e->rcv_desc().has_receive())
    {
      if (!e->rcv_desc().is_register_ipc())
	m = "long rcv";
      else if ((Config::deceit_bit_disables_switch && e->snd_desc().deceite())
	       || Config::disable_automatic_switch)
	m = "don't switch on rcv";
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
formatter_ipc_res(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ipc_res *e = static_cast<Tb_entry_ipc_res*>(tb);
  Mword error = e->result().error();
  const char *m;

  if (e->have_send())
    m = (e->rcv_desc().has_receive())
		? (e->rcv_desc().open_wait()) ? "rply" : "call"
		: "send";
  else
    m = (e->rcv_desc().open_wait()) ? "wait" : "recv";

  my_snprintf(buf, maxlen, "     %3x.%02x %s dope=%08x (%s%s) ",
			    tid.task(), tid.lthread(), m, e->result().raw(),
			    error ? "L4_IPC_" : "", e->result().str_error());

  if (e->rcv_desc().has_receive())
    {
      if (   !e->rcv_desc().open_wait()
	  || e->result().error() != L4_msgdope::RETIMEOUT)
	my_snprintf(buf, maxlen, "%x.%02x",
				 e->rcv_src().task(), e->rcv_src().lthread());
      if (!error)
	{
     	  if (e->result().fpage_received())
	    my_snprintf(buf, maxlen, "%s", 
		e->dword(1) & 1 ? " grant" : " map");
	  my_snprintf(buf, maxlen, 
	      " (%08x,%08x)", e->dword(0), e->dword(1));
	}
    }

  return maxlen;
}

// pagefault
static
unsigned
formatter_pf(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_pf *e = static_cast<Tb_entry_pf*>(tb);
  char eip_buf[16];

  sprintf(eip_buf, "%08x", e->eip());
  my_snprintf(buf, maxlen, "pf:  %3x.%02x pfa=%08x eip=%s (%c%c)",
      tid.task(), tid.lthread(), e->pfa(), e->eip() ? eip_buf : "unknown",
      e->error() & 2 ? (e->error() & 4 ? 'w' : 'W')
		     : (e->error() & 4 ? 'r' : 'R'),
      e->error() & 1 ? 'p' : '-');

  return maxlen;
}

// pagefault result
static
unsigned
formatter_pf_res(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_pf_res *e = static_cast<Tb_entry_pf_res*>(tb);

  // e->ret contains only an error code
  // e->err contains only up to 3 dwords
  my_snprintf(buf, maxlen, "     %3x.%02x pfa=%08x dope=%02x (%s%s) "
		"err=%04x (%s%s)",
	      tid.task(), tid.lthread(), e->pfa(),
	      e->ret().raw(), e->ret().error() ? "L4_IPC_" : "",
	      e->ret().str_error(),
	      e->err().raw(), e->err().error() ? "L4_IPC_" : "",
	      e->err().str_error());

  return maxlen;
}

// kernel event (enter_kdebug("*..."))
static
unsigned
formatter_ke(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ke *e = static_cast<Tb_entry_ke*>(tb);
  char eip_buf[16];
  char tid_buf[16];

  sprintf(eip_buf, " @%08x", e->eip());
  sprintf(tid_buf, "%3x.%02x: ", tid.task(), tid.lthread());
  my_snprintf(buf, maxlen, "ke:  %s\"%s\"%s",
      tid.is_invalid() ? "" : tid_buf, e->msg(), e->eip() ? eip_buf : "");

  return maxlen;
}

// kernel event (enter_kdebug_verb("*+...", eax, edx, ecx))
static
unsigned
formatter_ke_reg(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ke_reg *e = static_cast<Tb_entry_ke_reg*>(tb);

  char eip_buf[16];
  char tid_buf[16];
  sprintf(eip_buf, " @%08x", e->eip());
  sprintf(tid_buf, "%3x.%02x: ", tid.task(), tid.lthread());
  my_snprintf(buf, maxlen, "kr:  %s\"%s\" %08x %08x %08x%s",
      tid.is_invalid() ? "" : tid_buf, e->msg(), e->eax(), e->ecx(), e->edx(),
      e->eip() ? eip_buf : "");

  return maxlen;
}

// breakpoint
static
unsigned
formatter_bp(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_bp *e = static_cast<Tb_entry_bp*>(tb);

  my_snprintf(buf, maxlen, "b%c:  %3x.%02x at %08x ",
      "iwpa"[e->mode() & 3], tid.task(), tid.lthread(), e->eip());
  switch (e->mode() & 3)
    {
    case 1:
    case 3:
      switch (e->len())
	{
     	case 1:
	  my_snprintf(buf, maxlen, "[%08x]=%02x", e->addr(), e->value());
	  break;
	case 2:
	  my_snprintf(buf, maxlen, "[%08x]=%04x", e->addr(), e->value());
	  break;
	case 4:
	  my_snprintf(buf, maxlen, "[%08x]=%08x", e->addr(), e->value());
	  break;
	}
      break;
    case 2:
      my_snprintf(buf, maxlen, "[%08x]", e->addr());
      break;
    }

  return maxlen;
}

// context switch
static
unsigned
formatter_ctx_switch(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  char str[24];
  Tb_entry_ctx_sw *e = static_cast<Tb_entry_ctx_sw*>(tb);
  L4_uid dst = Thread::lookup(e->dest())->id();

  if (!Jdb_symbol::match_eip_to_symbol(e->kernel_eip(), 0 /*kernel*/,
				       str, sizeof(str)))
    sprintf(str, "%08x", e->kernel_eip());

  my_snprintf(buf, maxlen, "     %3x.%02x %c=> %3x.%02x eip %08x krnl %s",
      tid.task(), tid.lthread(), (e->kernel_esp() == 0) ? '!' : '=',
      dst.task(), dst.lthread(), e->eip(), str);

  return maxlen;
}

// unmap system call
static
unsigned
formatter_unmap(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_unmap *e = static_cast<Tb_entry_unmap*>(tb);

  my_snprintf(buf, maxlen, 
      "ump: %3x.%02x: at %08x size %dkB, %s %s, @%08x",
      tid.task(), tid.lthread(),
      e->fpage().page(), 1 << (e->fpage().size() - 10),
      e->mask() & 2 ? "flush" : "remap",
      e->mask() & 0x80000000 ? "all" : "other", e->eip());

  return maxlen;
}

// thread_ex_regs system call
static
unsigned
formatter_ex_regs(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_ex_regs *e = static_cast<Tb_entry_ex_regs*>(tb);

  my_snprintf(buf, maxlen, "exr: %3x.%02x at %08x: %02x ",
      tid.task(), tid.lthread(), e->eip(), e->lthread());
  if (e->new_eip() != 0xffffffff)
    my_snprintf(buf, maxlen, "eip %08x=>%08x ", e->old_eip(), e->new_eip());
  if (e->new_esp() != 0xffffffff)
    my_snprintf(buf, maxlen, "esp=%08x", e->new_esp());

  return maxlen;
}

// trap
static
unsigned
formatter_trap(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_trap *e = static_cast<Tb_entry_trap*>(tb);

  if (e->trapno() & 0x80)
    my_snprintf(buf, maxlen,
		"#%02x: %3x.%02x @%08x",
	      	e->trapno() & 0x7f, tid.task(), tid.lthread(), e->eip());
  else
    my_snprintf(buf, maxlen, 
	        "#%02x: %3x.%02x err=%04x @%08x cs=%04x esp=%08x fl=%08x",
	        e->trapno(), tid.task(), tid.lthread(), e->errno(), e->eip(),
	        e->cs(), e->esp(), e->eflags());

  return maxlen;
}

// trap
static
unsigned
formatter_sched(Tb_entry *tb, L4_uid tid, char *buf, int maxlen)
{
  Tb_entry_sched *e = static_cast<Tb_entry_sched*>(tb);

  my_snprintf (buf, maxlen, 
               "%3x.%02x -- timeslice %s -- id:%3u, prio:%3u, ticks:%3d/%-3u %s",
               tid.task(), tid.lthread(), e->mode() ? "load" : "save",
               e->id(), e->prio(), e->ticks_left(), e->timeslice(),
               e->ticks_left() <= 0 ? "PIPC" : "");

  return maxlen;
}


STATIC_INITIALIZER(init_formatters);

// register all available format functions
static FIASCO_INIT
void
init_formatters()
{
  Jdb_tbuf_output::register_ff(TBUF_PF, formatter_pf);
  Jdb_tbuf_output::register_ff(TBUF_PF_RES, formatter_pf_res);
  Jdb_tbuf_output::register_ff(TBUF_IPC, formatter_ipc);
  Jdb_tbuf_output::register_ff(TBUF_IPC_RES, formatter_ipc_res);
  Jdb_tbuf_output::register_ff(TBUF_KE, formatter_ke);
  Jdb_tbuf_output::register_ff(TBUF_KE_REG, formatter_ke_reg);
  Jdb_tbuf_output::register_ff(TBUF_UNMAP, formatter_unmap);
  Jdb_tbuf_output::register_ff(TBUF_SHORTCUT_FAILED, formatter_sc_failed);
  Jdb_tbuf_output::register_ff(TBUF_SHORTCUT_SUCCEEDED, formatter_ipc);
  Jdb_tbuf_output::register_ff(TBUF_CONTEXT_SWITCH, formatter_ctx_switch);
  Jdb_tbuf_output::register_ff(TBUF_EXREGS, formatter_ex_regs);
  Jdb_tbuf_output::register_ff(TBUF_BREAKPOINT, formatter_bp);
  Jdb_tbuf_output::register_ff(TBUF_TRAP, formatter_trap);
  Jdb_tbuf_output::register_ff(TBUF_SCHED, formatter_sched);
}

