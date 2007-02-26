IMPLEMENTATION [log]:

#include <alloca.h>
#include <cstring>
#include "config.h"
#include "idt.h"
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

// 
// IPC logging
// 

// called from interrupt gate
PUBLIC inline NOEXPORT void
Thread::sys_ipc_log()
{
  Entry_frame *regs = reinterpret_cast<Entry_frame*>(this->regs());
  Sys_ipc_frame *ipc_regs = reinterpret_cast<Sys_ipc_frame*>(this->regs());

  Mword entry_event_num = (Mword)-1;
  Unsigned8 have_send = ipc_regs->snd_desc().has_send();
  int do_log = Jdb_ipc_trace::log()
	       && Jdb_ipc_trace::check_restriction(id(), ipc_regs);

  if (do_log)
    {
#if GCC_VERSION < 302
      // older gcc cannot inline functions containing alloca()
      Tb_entry_ipc tbuf;
      Tb_entry_ipc *tb = static_cast<Tb_entry_ipc*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
				     : &tbuf);
#else
      Tb_entry_ipc *tb = static_cast<Tb_entry_ipc*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
					   : alloca(sizeof(Tb_entry_ipc)));
#endif
      tb->set(this, regs->eip, ipc_regs);
	
      entry_event_num = tb->number();

      if (EXPECT_TRUE(Jdb_ipc_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else 
	Jdb_tbuf::direct_log_entry(tb, "IPC");
    }

  // now pass control to regular sys_ipc()
  register bool success = ipc_short_cut_wrapper();
  state_change_dirty (~Thread_ipc_mask, 0);

  if (!success)
    // shortcut failed, try normal path
    sys_ipc_wrapper();

  if (Jdb_ipc_trace::log() && Jdb_ipc_trace::log_result() && do_log)
    {
#if GCC_VERSION < 302
      // older gcc cannot inline functions containing alloca()
      Tb_entry_ipc_res tbuf;
      Tb_entry_ipc_res *tb = static_cast<Tb_entry_ipc_res*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
					       : &tbuf);
#else
      Tb_entry_ipc_res *tb = static_cast<Tb_entry_ipc_res*>
	(EXPECT_TRUE(Jdb_ipc_trace::log_buf()) ? Jdb_tbuf::new_entry()
					    : alloca(sizeof(Tb_entry_ipc_res)));
#endif
      tb->set(this, regs->eip, ipc_regs, regs->eax, entry_event_num, have_send);

      if (EXPECT_TRUE(Jdb_ipc_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "IPC result");
    }
}

extern "C" void sys_ipc_log_wrapper(void)
{
  current_thread()->sys_ipc_log();
}


// 
// IPC tracing
// 

PUBLIC inline NOEXPORT void
Thread::sys_ipc_trace()
{
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(this->regs());
  Sys_ipc_frame *regs = reinterpret_cast<Sys_ipc_frame*>(this->regs());

  L4_snd_desc snd_desc = regs->snd_desc();
  L4_rcv_desc rcv_desc = regs->rcv_desc();
  L4_uid      snd_dest = regs->snd_dest();

  Unsigned64 orig_tsc = Cpu::rdtsc();

  // first try the fastpath, then the "slowpath"
  register bool success = ipc_short_cut_wrapper();

  if (!success)
    sys_ipc_wrapper();

  // kernel is locked here => no Lock_guard <...> needed
  Tb_entry_ipc_trace *tb = 
    static_cast<Tb_entry_ipc_trace*>(Jdb_tbuf::new_entry());

  tb->set(this, ef->eip, orig_tsc, snd_dest, regs->rcv_source(), regs->eax,
	  ((!snd_desc.has_send() || !snd_desc.msg())
	      ?  static_cast<Unsigned8>(snd_desc.raw())
	      : (static_cast<Unsigned8>(snd_desc.raw()) & 3) | 4),
	   (!rcv_desc.has_receive() || rcv_desc.is_register_ipc())
	      ?  static_cast<Unsigned8>(rcv_desc.raw())
	      : (static_cast<Unsigned8>(rcv_desc.raw()) & 3) | 4);

  Jdb_tbuf::commit_entry();
}

extern "C" void sys_ipc_trace_wrapper(void)
{
  current_thread()->sys_ipc_trace();
}


// 
// Page-fault logging
// 

void 
Thread::page_fault_log(Address pfa, unsigned error_code, unsigned eip)
{
  if (Jdb_pf_trace::check_restriction(current_thread()->id(), pfa))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      
      Tb_entry_pf *tb = static_cast<Tb_entry_pf*>
	(EXPECT_TRUE(Jdb_pf_trace::log_buf()) ? Jdb_tbuf::new_entry()
				    : alloca(sizeof(Tb_entry_pf)));
      tb->set(this, eip, pfa, error_code);

      if (EXPECT_TRUE(Jdb_pf_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "PF");
    }
}

/** L4 system call fpage_unmap.  */
PUBLIC inline NOEXPORT void
Thread::sys_fpage_unmap_log()
{
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(this->regs());

  if (Jdb_unmap_trace::check_restriction(current_thread()->id(), 
				 regs()->eax & Config::PAGE_MASK))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      
#if GCC_VERSION < 302
      // older gcc cannot inline functions containing alloca()
      Tb_entry_unmap tbuf;
      Tb_entry_unmap *tb = static_cast<Tb_entry_unmap*>
	(EXPECT_TRUE(Jdb_unmap_trace::log_buf()) ? Jdb_tbuf::new_entry() 
					       : &tbuf);
#else
      Tb_entry_unmap *tb = static_cast<Tb_entry_unmap*>
	(EXPECT_TRUE(Jdb_unmap_trace::log_buf()) ? Jdb_tbuf::new_entry() 
					     : alloca(sizeof(Tb_entry_unmap)));
#endif
      tb->set(this, ef->eip, regs()->eax, regs()->ecx, false);

      if (EXPECT_TRUE(Jdb_unmap_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "UNMAP");
    }

  sys_fpage_unmap_wrapper();
}

extern "C" void sys_fpage_unmap_log_wrapper(void)
{
  current_thread()->sys_fpage_unmap_log();
}

