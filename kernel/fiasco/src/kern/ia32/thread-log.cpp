INTERFACE:

EXTENSION class Thread
{
public:
  static bool log_ipc;
  static bool log_ipc_to_buf;
  static bool log_ipc_result;
  static bool log_page_fault;
  static bool log_pf_to_buf;
  static bool log_unmap;
  static bool log_unmap_to_buf;
  static bool trace_ipc;
  static bool slow_ipc;
  static bool cpath_ipc;
};

IMPLEMENTATION [log]:

#include <alloca.h>
#include <cstring>
#include "idt.h"
#include "jdb_trace.h"
#include "jdb_tbuf.h"
#include "types.h"
#include "cpu_lock.h"

// OSKIT crap
#include "undef_oskit.h"

bool Thread::log_ipc;
bool Thread::log_ipc_to_buf;
bool Thread::log_ipc_result;
bool Thread::log_page_fault;
bool Thread::log_pf_to_buf;
bool Thread::log_unmap;
bool Thread::log_unmap_to_buf;
bool Thread::trace_ipc;
bool Thread::slow_ipc;
bool Thread::cpath_ipc;

// 
// System-call logging
// 

bool log_system_call;

static char * const syscall_names[] =
{
  "IPC", "IDN", "FPU", "SWI", "SCH", "EXR", "TNW"
};

// shared buffer protected by save_disable_irq()/restore_irqs()


PUBLIC inline NOEXPORT
void 
Thread::syscall_log(Syscall_frame * /*regs*/, unsigned number, 
		    bool enter, unsigned retval)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  printf("*S%c[%x.%x %s",
	 enter ? 'c' : 'r',
	 id().task(), id().lthread(),
	 syscall_names[number]);

  if (!enter) 
    printf(" -> %x", retval);

  printf("] ");
}

extern "C" void
thread_syscall_log_begin(Thread *t, Syscall_frame *regs, unsigned number)
{
  t->syscall_log(regs, number, true,0);
}

extern "C" void
thread_syscall_log_end(unsigned retval,
		       Thread *t, Syscall_frame *regs, unsigned number)
{
  t->syscall_log(regs, number, false,retval);
}

// 
// IPC logging
// 

// called from interrupt gate
PUBLIC
Mword Thread::sys_ipc_log(Syscall_frame *i_regs)
{
  Entry_frame *regs = reinterpret_cast<Entry_frame*>(i_regs);
  Sys_ipc_frame *ipc_regs = static_cast<Sys_ipc_frame*>(i_regs);

  Mword entry_event_num = (Mword)-1;
  Unsigned8 have_send = ipc_regs->snd_desc().has_send();
  int do_log = log_ipc && Jdb_trace::check_ipc_res(id(), ipc_regs);

  // only necessary if kernel lock has different semantic than cli/sti
  cpu_lock.lock();
  
  if (do_log)
    {
      Tb_entry_ipc *tb = static_cast<Tb_entry_ipc*>
	(log_ipc_to_buf ? Jdb_tbuf::new_entry()
			: alloca(sizeof(Tb_entry_ipc)));
      tb->set(this, regs->eip, ipc_regs);
	
      entry_event_num = tb->number();

      if (log_ipc_to_buf)
	Jdb_tbuf::commit_entry();
      else 
	Jdb_tbuf::direct_log_entry(tb, "IPC");
    }

  // now pass control to regular sys_ipc()
  Mword ret;

  bool success = ipc_short_cut(ipc_regs);
  state_change_dirty (~Thread_ipc_mask, 0);

  cpu_lock.clear();
  
  ret = regs->eax;
  if (!success)
    // shortcut failed, try normal path
    ret = sys_ipc(regs);

  if (log_ipc && log_ipc_result && do_log)
    {
      cpu_lock.lock();
      
      Tb_entry_ipc_res *tb = static_cast<Tb_entry_ipc_res*>
	(log_ipc_to_buf ? Jdb_tbuf::new_entry()
			: alloca(sizeof(Tb_entry_ipc_res)));
      tb->set(this, regs->eip, ipc_regs, ret, entry_event_num, have_send);

      if (log_ipc_to_buf)
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "IPC result");
	
      cpu_lock.clear();
    }

  return ret;
}


// 
// IPC tracing
// 

PUBLIC
unsigned
Thread::sys_ipc_trace(Syscall_frame *i_regs)
{
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(i_regs);
  Sys_ipc_frame *regs = static_cast<Sys_ipc_frame*>(i_regs);

  L4_snd_desc snd_desc = regs->snd_desc();
  L4_rcv_desc rcv_desc = regs->rcv_desc();
  L4_uid      snd_dest = regs->snd_dest();

  // call the fastpath with cli
  cpu_lock.lock();
  Unsigned64 orig_tsc = Cpu::rdtsc();

  // first try the fastpath, then the "slowpath"
  register bool success = ipc_short_cut(regs);
  unsigned ret = regs->eax;

  if (!success)
    {
      // slowpath is called with interrupts enabled
      cpu_lock.clear();
      ret = sys_ipc(regs);
      cpu_lock.lock();
    }

  // kernel is locked here => no Lock_guard <...> needed
  Tb_entry_ipc_trace *tb = 
    static_cast<Tb_entry_ipc_trace*>(Jdb_tbuf::new_entry());

  tb->set(this, ef->eip, orig_tsc, snd_dest, regs->rcv_source(), ret,
	  ((!snd_desc.has_send() || !snd_desc.msg())
	      ?  static_cast<Unsigned8>(snd_desc.raw())
	      : (static_cast<Unsigned8>(snd_desc.raw()) & 3) | 4),
	   (!rcv_desc.has_receive() || rcv_desc.is_register_ipc())
	      ?  static_cast<Unsigned8>(rcv_desc.raw())
	      : (static_cast<Unsigned8>(rcv_desc.raw()) & 3) | 4);

  Jdb_tbuf::commit_entry();

  // now unlock the kernel and return
  cpu_lock.clear();  
  return ret;
}


extern "C" void sys_ipc_entry_log (void);
extern "C" void do_sysenter_log (void);
extern "C" void sys_ipc_entry_c (void);
extern "C" void sys_ipc_entry (void);
extern "C" void do_sysenter_c (void);
extern "C" void do_sysenter (void);

static
void
Thread::set_ipc_vector()
{
  void (*int30_entry)(void);
  void (*sysenter_entry)(void);

  if (trace_ipc || slow_ipc || log_ipc)
    {
      int30_entry    = sys_ipc_entry_log;
      sysenter_entry = do_sysenter_log;
    }
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
  else if (cpath_ipc)
    {
      int30_entry    = sys_ipc_entry_c;
      sysenter_entry = do_sysenter_c;
    }
  else
    {
      int30_entry    = sys_ipc_entry;
      sysenter_entry = do_sysenter;
    }
#else
  else
    {
      int30_entry    = sys_ipc_entry_c;
      sysenter_entry = do_sysenter_c;
    }
#endif

  Idt::set_vector (0x30, (unsigned) int30_entry, true);

  Cpu::set_sysenter(sysenter_entry);

  if (trace_ipc)
    syscall_table[0] = &Thread::sys_ipc_trace;
  else if (log_ipc && !slow_ipc)
    syscall_table[0] = &Thread::sys_ipc_log;
  else
    syscall_table[0] = &Thread::sys_ipc;
}

// 
// Page-fault logging
// 

void 
Thread::page_fault_log(Address pfa, unsigned error_code, unsigned eip)
{
  if (Jdb_trace::check_pf_res(current_thread()->id(), pfa))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      
      Tb_entry_pf *tb = static_cast<Tb_entry_pf*>
	(log_pf_to_buf ? Jdb_tbuf::new_entry()
		       : alloca(sizeof(Tb_entry_pf)));
      tb->set(this, eip, pfa, error_code);

      if (log_pf_to_buf)
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "PF");
    }
}

/** L4 system call fpage_unmap.  */
PUBLIC
unsigned
Thread::sys_fpage_unmap_log(Syscall_frame *regs)
{
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);

  if (Jdb_trace::check_unmap_res(current_thread()->id(), 
				 regs->eax & Config::PAGE_MASK))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      
      Tb_entry_unmap *tb = static_cast<Tb_entry_unmap*>
	(log_unmap_to_buf ? Jdb_tbuf::new_entry() 
			  : alloca(sizeof(Tb_entry_unmap)));
      tb->set(this, ef->eip, regs->eax, regs->ecx, false);
      if (log_unmap_to_buf)
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "UNMAP");
    }

  bool ret = sys_fpage_unmap(regs);

  return ret;
}

static
void
Thread::set_unmap_vector()
{
  if (log_unmap)
    syscall_table[2] = &Thread::sys_fpage_unmap_log;
  else
    syscall_table[2] = &Thread::sys_fpage_unmap;
}
