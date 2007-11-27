IMPLEMENTATION [log]:

#include <alloca.h>
#include <cstring>
#include "config.h"
#include "jdb_trace.h"
#include "jdb_tbuf.h"
#include "types.h"
#include "cpu_lock.h"

PUBLIC static inline NEEDS["jdb_trace.h"]
int
Thread::log_page_fault()
{
  return Jdb_pf_trace::log();
}

/** IPC logging.
    called from interrupt gate.
 */
PUBLIC inline NOEXPORT ALWAYS_INLINE
void
Thread::sys_ipc_log()
{
  Entry_frame   *regs      = reinterpret_cast<Entry_frame*>(this->regs());
  Sys_ipc_frame *ipc_regs  = reinterpret_cast<Sys_ipc_frame*>(this->regs());

  Mword entry_event_num    = (Mword)-1;
  Unsigned8 have_snd       = ipc_regs->snd_desc().has_snd();
  Unsigned8 is_next_period = ipc_regs->next_period();
  int do_log               = Jdb_ipc_trace::log() &&
				Jdb_ipc_trace::check_restriction (id(),
					 get_task (id()),
					 ipc_regs,
					 get_task (ipc_regs->snd_dst()));

  if (Jdb_nextper_trace::log() && is_next_period)
    {
      Tb_entry_ipc *tb = static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());
      tb->set(this, regs->ip_syscall_page_user(), ipc_regs, sched_context()->left());
      Jdb_tbuf::commit_entry();
      goto skip_ipc_log;
    }

  if (do_log)
    {
      Tb_entry_ipc *tb = static_cast<Tb_entry_ipc*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
					   : alloca(sizeof(Tb_entry_ipc)));
      tb->set(this, regs->ip_syscall_page_user(), ipc_regs, sched_context()->left());

      entry_event_num = tb->number();

      if (EXPECT_TRUE(Jdb_ipc_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "IPC");
    }

skip_ipc_log:

  // now pass control to regular sys_ipc()
  ipc_short_cut_wrapper();

  if (Jdb_nextper_trace::log() && is_next_period)
    {
      Tb_entry_ipc_res *tb =
	    static_cast<Tb_entry_ipc_res*>(Jdb_tbuf::new_entry());
      tb->set(this, regs->ip_syscall_page_user(), ipc_regs, ipc_regs->snd_desc().raw(),
	      entry_event_num, have_snd, is_next_period);
      Jdb_tbuf::commit_entry();
      goto skip_ipc_res_log;
    }

  if (Jdb_ipc_trace::log() && Jdb_ipc_trace::log_result() && do_log)
    {
      Tb_entry_ipc_res *tb = static_cast<Tb_entry_ipc_res*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
					    : alloca(sizeof(Tb_entry_ipc_res)));
      tb->set(this, regs->ip_syscall_page_user(), ipc_regs, ipc_regs->snd_desc().raw(),
	      entry_event_num, have_snd, is_next_period);

      if (EXPECT_TRUE(Jdb_ipc_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "IPC result");
    }

skip_ipc_res_log:
  ;
}

/** IPC tracing.
 */
PUBLIC inline NOEXPORT ALWAYS_INLINE
void
Thread::sys_ipc_trace()
{
  Entry_frame *ef      = reinterpret_cast<Entry_frame*>(this->regs());
  Sys_ipc_frame *regs  = reinterpret_cast<Sys_ipc_frame*>(this->regs());

  L4_snd_desc snd_desc = regs->snd_desc();
  L4_rcv_desc rcv_desc = regs->rcv_desc();
  L4_uid      snd_dst  = regs->snd_dst();

  Unsigned64 orig_tsc  = Cpu::rdtsc();

  // first try the fastpath, then the "slowpath"
  ipc_short_cut_wrapper();

  // kernel is locked here => no Lock_guard <...> needed
  Tb_entry_ipc_trace *tb =
    static_cast<Tb_entry_ipc_trace*>(Jdb_tbuf::new_entry());

  tb->set(this, ef->ip_syscall_page_user(), orig_tsc, snd_dst,
          regs->rcv_src(), snd_desc.raw(),
	  ((!snd_desc.has_snd() || !snd_desc.msg())
	      ?  static_cast<Unsigned8>(snd_desc.raw())
	      : (static_cast<Unsigned8>(snd_desc.raw()) & 3) | 4),
	   (!rcv_desc.has_receive() || rcv_desc.is_register_ipc())
	      ?  static_cast<Unsigned8>(rcv_desc.raw())
	      : (static_cast<Unsigned8>(rcv_desc.raw()) & 3) | 4);

  Jdb_tbuf::commit_entry();
}

/** Page-fault logging.
 */
void
Thread::page_fault_log(Address pfa, unsigned error_code, unsigned long eip)
{
  if (Jdb_pf_trace::check_restriction(current_thread()->id(), pfa))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);

      Tb_entry_pf *tb = static_cast<Tb_entry_pf*>
	(EXPECT_TRUE(Jdb_pf_trace::log_buf()) ? Jdb_tbuf::new_entry()
				    : alloca(sizeof(Tb_entry_pf)));
      tb->set(this, eip, pfa, error_code, current()->space());

      if (EXPECT_TRUE(Jdb_pf_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "PF");
    }
}


extern "C" void sys_ipc_log_wrapper(void)
{
  current_thread()->sys_ipc_log();
}

extern "C" void sys_ipc_trace_wrapper(void)
{
  current_thread()->sys_ipc_trace();
}

