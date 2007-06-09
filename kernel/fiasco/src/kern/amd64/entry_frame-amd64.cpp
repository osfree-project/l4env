/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE[amd64]:

#include "types.h"

EXTENSION class Syscall_frame
{
protected:
  Mword    _r15;
  Mword    _r14;
  Mword    _r13;
  Mword    _r12;
  Mword    _r11;
  Mword    _r10;
  Mword     _r9;
  Mword     _r8;
  Mword    _rcx;
  Mword    _rdx;
  Mword    _rsi;
  Mword    _rdi;
  Mword    _rbx;
  Mword    _rbp;
  Mword    _rax;
};

EXTENSION class Return_frame
{
private:
  Mword    _rip;
  Mword     _cs;
  Mword _rflags;
  Mword    _rsp;
  Mword     _ss;
};

IMPLEMENTATION[ux,amd64]:

//---------------------------------------------------------------------------
// basic Entry_frame methods for IA32
// 
IMPLEMENT inline
Address
Return_frame::ip() const
{ return _rip; }

IMPLEMENT inline
void
Return_frame::ip(Mword ip)
{ _rip = ip; }

IMPLEMENT inline
Address
Return_frame::sp() const
{ return _rsp; }

IMPLEMENT inline
void
Return_frame::sp(Mword sp)
{ _rsp = sp; }

PUBLIC inline
Mword 
Return_frame::flags() const
{ return _rflags; }

PUBLIC inline
void
Return_frame::flags(Mword flags)
{ _rflags = flags; }

PUBLIC inline
Mword
Return_frame::cs() const
{ return _cs; }

PUBLIC inline
void
Return_frame::cs(Mword cs)
{ _cs = cs; }

PUBLIC inline
Mword
Return_frame::ss() const
{ return _ss; }

PUBLIC inline
void
Return_frame::ss(Mword ss)
{ _ss = ss; }

//---------------------------------------------------------------------------
IMPLEMENTATION [ux,amd64]:

//---------------------------------------------------------------------------
// IPC frame methods for IA32 (x0 and v2)
// 
IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dst() const
{ return _rsi; }
 
IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{ return _rsi - 1; }
 
IMPLEMENT inline 
void Sys_ipc_frame::snd_desc(Mword w)
{ _rax = w; }
 
IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{ return _rax; }

IMPLEMENT inline
Mword Sys_ipc_frame::has_snd() const
{ return snd_desc().has_snd(); }

IMPLEMENT inline 
L4_timeout_pair Sys_ipc_frame::timeout() const
{ return L4_timeout_pair(_rdi); }

IMPLEMENT inline
L4_rcv_desc Sys_ipc_frame::rcv_desc() const
{ return _rbp; }

IMPLEMENT inline
void Sys_ipc_frame::rcv_desc(L4_rcv_desc d)
{ _rbp = d.raw(); }

IMPLEMENT inline
L4_msgdope Sys_ipc_frame::msg_dope() const
{ return L4_msgdope(_rax); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error(Mword e)
{ reinterpret_cast<L4_msgdope&>(_rax).error(e); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_snd_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_rcv_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope(L4_msgdope d)
{ _rax = d.raw(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_combine(Ipc_err e)
{
  L4_msgdope m(_rax);
  m.combine(e);
  _rax = m.raw();
}

//---------------------------------------------------------------------------
// ID-nearest frame methods for IA32 (v2 and x0)
// 
IMPLEMENT inline
void Sys_id_nearest_frame::type(Mword type)
{ _rax = type; }

//---------------------------------------------------------------------------
// ex-regs frame methods for IA32 (v2 and x0)
// 
// XXX we can send a normal l4 UID
IMPLEMENT inline
LThread_num Sys_ex_regs_frame::lthread() const
{ return _rax & (1UL << 7) - 1; }

// XXX we can send a normal l4 UID
IMPLEMENT inline
Task_num Sys_ex_regs_frame::task() const
{
  return (_rax >> 7) & ((1UL << 11) - 1);
}

IMPLEMENT inline
Mword Sys_ex_regs_frame::trigger_exception() const
{ return _rax & (1 << 28); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::alien() const
{ return _rax & (1UL << 29); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::no_cancel() const
{ return _rax & (1UL << 30); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::sp() const
{ return _rcx; }

IMPLEMENT inline
Mword Sys_ex_regs_frame::ip() const
{ return _rdx; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_eflags(Mword oefl)
{ _rax = oefl; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_sp(Mword osp)
{ _rcx = osp; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_ip(Mword oip)
{ _rdx = oip; }
//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64 && caps]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_ex_regs_frame::cap_handler(const Utcb* utcb) const
{
  if (! (_rax & (1 << 27)))
    return L4_uid::Invalid;

  return utcb->values[1];
}

IMPLEMENT inline NEEDS["utcb.h"]
void Sys_ex_regs_frame::old_cap_handler(L4_uid const &id, Utcb* utcb)
{
  utcb->values[1] = id.raw();
}


//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64 && !caps]:

// If UTCBs are not available, changing or requesting the
// task-capability fault handler is not supported on IA32.

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::cap_handler(const Utcb* /*utcb*/) const
{
  return L4_uid::Invalid;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_cap_handler(L4_uid const &/*id*/, Utcb* /*utcb*/)
{
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64]:
//---------------------------------------------------------------------------
// thread-switch frame for IA32 (v2 and x0)
//

IMPLEMENT inline
Mword
Sys_thread_switch_frame::id() const
{ return _rax; }

IMPLEMENT inline
void
Sys_thread_switch_frame::left (Unsigned64 t)
{ _rcx = t; }

IMPLEMENT inline
void
Sys_thread_switch_frame::ret (Mword val)
{ _rax = val; }

//---------------------------------------------------------------------------
// thread-schedule frame for IA32 (v2 and x0)
//

IMPLEMENT inline 
L4_sched_param
Sys_thread_schedule_frame::param() const
{ return L4_sched_param (_rax); }

IMPLEMENT inline
void
Sys_thread_schedule_frame::old_param (L4_sched_param op)
{ _rax = op.raw(); }

IMPLEMENT inline
Unsigned64
Sys_thread_schedule_frame::time() const
{ return (Unsigned64) _rcx; }

IMPLEMENT inline
void
Sys_thread_schedule_frame::time (Unsigned64 t)
{ _rcx = t; }

//---------------------------------------------------------------------------
// Sys-unmap frame for IA32 (v2 and x0)
//

IMPLEMENT inline
L4_fpage Sys_unmap_frame::fpage() const
{ return L4_fpage(_rax); }

IMPLEMENT inline
Mword Sys_unmap_frame::map_mask() const
{ return _rcx; }

IMPLEMENT inline
bool Sys_unmap_frame::downgrade() const
{ return !(_rcx & 2); }

IMPLEMENT inline
bool Sys_unmap_frame::no_unmap() const
{ return _rcx & 4; }

IMPLEMENT inline
bool Sys_unmap_frame::reset_references() const
{ return _rcx & 8; }

IMPLEMENT inline
void
Sys_unmap_frame::ret (Mword status)
{
  // keep the fpage part of EAX intact
  _rax = (_rax & ~L4_fpage::Status_mask) | status;
}

IMPLEMENT inline
bool Sys_unmap_frame::self_unmap() const
{ return _rcx & 0x80000000; }

IMPLEMENT inline
Task_num Sys_unmap_frame::restricted() const
{ return (_rcx & 0x7ff00) >> 8; }

//---------------------------------------------------------------------------
// Sys-task-new frame for IA32 (v2 and x0)
//

IMPLEMENT inline 
Mword Sys_task_new_frame::trigger_exception() const 
{ return _rax & (1 << 30); }

IMPLEMENT inline 
Mword Sys_task_new_frame::alien() const 
{ return _rax & (1 << 31); }

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{ return _rax & ~(7 << 30); }

IMPLEMENT inline 
Mword Sys_task_new_frame::sp() const
{ return _rcx; }

IMPLEMENT inline 
Mword Sys_task_new_frame::ip() const
{ return _rdx; }
 
IMPLEMENT inline
Mword Sys_task_new_frame::has_pager() const
{ return _r8; }

IMPLEMENT inline
Mword Sys_task_new_frame::extra_args() const
{ return _rax & (1 << 29); }

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64 && caps]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_task_new_frame::cap_handler(const Utcb* utcb) const
{
  if (! extra_args())
    return L4_uid::Invalid;

  return utcb->values[1];
}

IMPLEMENT inline NEEDS["utcb.h"]
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb* utcb) const
{
  if (! extra_args())
    return L4_quota_desc(0);

  return L4_quota_desc(utcb->values[4]);
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64 && !caps]:

#include "utcb.h"

// If UTCBs are not available, changing or requesting the
// task-capability fault handler is not supported on IA32 / AMD64.

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_task_new_frame::cap_handler(const Utcb* /*utcb*/) const
{
  return L4_uid::Invalid;
}

IMPLEMENT inline 
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb*) const
{
  return L4_quota_desc(0);
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [amd64 && v2]:

IMPLEMENT inline
Mword Sys_ipc_frame::next_period() const
{ return false; }

IMPLEMENT inline
L4_msg_tag Sys_ipc_frame::tag() const
{ return L4_msg_tag(_rcx); }

IMPLEMENT inline
void Sys_ipc_frame::tag(L4_msg_tag const &tag)
{ _rcx = tag.raw(); }

IMPLEMENT inline 
void Sys_ipc_frame::rcv_src(L4_uid const &id) 
{ _rsi = id.raw(); }

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_src() const
{ return L4_uid(_rsi); }

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dst() const
{ return L4_uid(_rsi); }

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word(unsigned index) const
{
  switch(index)
    {
    case 0:
      return _rdx;
    case 1:
      return _r8;
    default:
      return 0;
    }
}

IMPLEMENT inline 
void Sys_ipc_frame::set_msg_word(unsigned index, Mword value)
{
  switch(index)
    {
    case 0:
      _rdx = value;
      break;
    case 1:
      _r8 = value;
      break;
    default:
      break;
    }
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{ return 2; }

IMPLEMENT inline
void Sys_ipc_frame::copy_msg(Sys_ipc_frame *to) const
{ 
  // hint for gcc to prevent stall
  Mword tmp_rdx = _rdx, tmp_rbx = _r8;
  to->_rdx = tmp_rdx;
  to->_r8 = tmp_rbx;
}

//Entry_id_nearest_data::-------------------------------------------------
IMPLEMENT inline
L4_uid Sys_id_nearest_frame::dst() const
{ return L4_uid(_rsi); }

IMPLEMENT inline
void Sys_id_nearest_frame::nearest(L4_uid const &id)
{ _rsi = id.raw(); }

//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::preempter() const
{ return L4_uid(_r8); }

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::pager() const
{ return L4_uid(_rsi); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_preempter(L4_uid const &id)
{ _r8 = id.raw(); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager(L4_uid const &id)
{ _rsi = id.raw(); }


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline
L4_uid
Sys_thread_switch_frame::dst() const
{ return L4_uid(_rsi); }

//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::preempter() const 
{ return L4_uid(_r8); }

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::dst() const
{ return L4_uid(_rsi); }

IMPLEMENT inline 
void
Sys_thread_schedule_frame::old_preempter (L4_uid const &id)
{ _r8 = id.raw(); }

IMPLEMENT inline
void 
Sys_thread_schedule_frame::partner (L4_uid const &id)
{ _rsi = id.raw(); }

//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline
L4_uid Sys_task_new_frame::new_chief() const
{ return L4_uid(_rax & ~(1UL << 31)); }

IMPLEMENT inline
L4_uid Sys_task_new_frame::pager() const
{ return L4_uid(_r8); }

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dst() const
{ return L4_uid(_rsi); }

IMPLEMENT inline
void Sys_task_new_frame::new_taskid(L4_uid const &id)
{ _rsi = id.raw(); }


//--Sys_u_lock_frame------------------------------------------------

IMPLEMENT inline
Sys_u_lock_frame::Op Sys_u_lock_frame::op() const
{ return (Op)_rax; }

IMPLEMENT inline
unsigned long Sys_u_lock_frame::lock() const
{ return _rdx; }

IMPLEMENT inline
void Sys_u_lock_frame::result(unsigned long res)
{ _rax = res; }

IMPLEMENT inline
L4_timeout Sys_u_lock_frame::timeout() const
{ return L4_timeout(_rdi); }

IMPLEMENT inline
L4_semaphore *Sys_u_lock_frame::semaphore() const
{ return (L4_semaphore*)_rcx; }


IMPLEMENTATION[pl0_hack]:

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::command() const
{ return _rax; }

IMPLEMENT inline
L4_uid Sys_thread_privctrl_frame::dst() const
{ return L4_uid(_rdx, 0); }

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::entry_func() const
{ return _rdx; }

IMPLEMENT inline
void Sys_thread_privctrl_frame::ret_val(Mword v)
{ _rax = v; }
