/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE[ia32,ux]:

#include "types.h"

EXTENSION class Syscall_frame
{
protected:
  Mword             _ecx;
  Mword             _edx;
  Mword             _esi;
  Mword             _edi;
  Mword             _ebx;
  Mword             _ebp;
  Mword             _eax;
};

EXTENSION class Return_frame
{
private:
  Mword             _eip;
  Unsigned16  _cs, __csu;
  Mword          _eflags;
  Mword             _esp;
  Unsigned16  _ss, __ssu;
};

IMPLEMENTATION[ia32,ux]:

//---------------------------------------------------------------------------
// basic Entry_frame methods for IA32
// 
IMPLEMENT inline
Address
Return_frame::ip() const
{ return _eip; }

IMPLEMENT inline
void
Return_frame::ip(Mword ip)
{ _eip = ip; }

IMPLEMENT inline
Address
Return_frame::sp() const
{ return _esp; }

IMPLEMENT inline
void
Return_frame::sp(Mword sp)
{ _esp = sp; }

PUBLIC inline
Mword 
Return_frame::flags() const
{ return _eflags; }

PUBLIC inline
void
Return_frame::flags(Mword flags)
{ _eflags = flags; }

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
IMPLEMENTATION [ia32,ux]:

//---------------------------------------------------------------------------
// IPC frame methods for IA32 (x0 and v2)
// 
IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dst() const
{ return _esi; }
 
IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{ return _esi -1; }
 
IMPLEMENT inline 
void Sys_ipc_frame::snd_desc(Mword w)
{ _eax = w; }
 
IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{ return _eax; }

IMPLEMENT inline
Mword Sys_ipc_frame::has_snd() const
{ return snd_desc().has_snd(); }

IMPLEMENT inline 
L4_timeout Sys_ipc_frame::timeout() const
{ return L4_timeout(_ecx); }

IMPLEMENT inline
L4_rcv_desc Sys_ipc_frame::rcv_desc() const
{ return _ebp; }

IMPLEMENT inline
void Sys_ipc_frame::rcv_desc(L4_rcv_desc d)
{ _ebp = d.raw(); }

IMPLEMENT inline
L4_msgdope Sys_ipc_frame::msg_dope() const
{ return L4_msgdope(_eax); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error(Mword e)
{ reinterpret_cast<L4_msgdope&>(_eax).error(e); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_snd_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_rcv_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope(L4_msgdope d)
{ _eax = d.raw(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_combine(Ipc_err e)
{
  L4_msgdope m(_eax);
  m.combine(e);
  _eax = m.raw();
}

//---------------------------------------------------------------------------
// ID-nearest frame methods for IA32 (v2 and x0)
// 
IMPLEMENT inline
void Sys_id_nearest_frame::type(Mword type)
{ _eax = type; }

//---------------------------------------------------------------------------
// ex-regs frame methods for IA32 (v2 and x0)
// 
IMPLEMENT inline
Task_num Sys_ex_regs_frame::lthread() const
{ return _eax & (1 << 7) - 1; }

IMPLEMENT inline
Task_num Sys_ex_regs_frame::task() const
{ return (_eax >> 7) & ((1 << 11) - 1); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::trigger_exception() const
{ return _eax & (1 << 28); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::alien() const
{ return _eax & (1 << 29); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::no_cancel() const
{ return _eax & (1 << 30); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::sp() const
{ return _ecx; }

IMPLEMENT inline
Mword Sys_ex_regs_frame::ip() const
{ return _edx; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_eflags(Mword oefl)
{ _eax = oefl; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_sp(Mword osp)
{ _ecx = osp; }

IMPLEMENT inline
void Sys_ex_regs_frame::old_ip(Mword oip)
{ _edx = oip; }


//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [{ia32,ux}-caps-utcb]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_ex_regs_frame::cap_handler(const Utcb* utcb) const
{
  if (! (_eax & (1 << 27)))
    return L4_uid::Invalid;

  return utcb->values[2];
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [{ia32,ux}-caps-utcb-v2]:

IMPLEMENT inline NEEDS["utcb.h"]
void Sys_ex_regs_frame::old_cap_handler(L4_uid id, Utcb* utcb)
{
  utcb->values[2] = id.raw();
  utcb->values[3] = (id.raw() >> 32);
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [{ia32,ux}-caps-utcb-x0]:

IMPLEMENT inline NEEDS["utcb.h"]
void Sys_ex_regs_frame::old_cap_handler(L4_uid id, Utcb* utcb)
{
  utcb->values[2] = id.raw();
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [{ia32,ux}-!{caps-utcb}]:

#include "utcb.h"

// If UTCBs are not available, changing or requesting the
// task-capability fault handler is not supported on IA32.

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_ex_regs_frame::cap_handler(const Utcb* /*utcb*/) const
{
  return L4_uid::Invalid;
}

IMPLEMENT inline NEEDS["utcb.h"]
void Sys_ex_regs_frame::old_cap_handler(L4_uid /*id*/, Utcb* /*utcb*/)
{
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [ia32,ux]:

//---------------------------------------------------------------------------
// thread-switch frame for IA32 (v2 and x0)
//

IMPLEMENT inline
Mword
Sys_thread_switch_frame::id() const
{ return _eax; }

IMPLEMENT inline
void
Sys_thread_switch_frame::left (Unsigned64 t)
{
  _ecx = t;
  _edx = t >> 32;
}

IMPLEMENT inline
void
Sys_thread_switch_frame::ret (Mword val)
{ _eax = val; }

//---------------------------------------------------------------------------
// thread-schedule frame for IA32 (v2 and x0)
//

IMPLEMENT inline
L4_sched_param
Sys_thread_schedule_frame::param() const
{ return L4_sched_param (_eax); }

IMPLEMENT inline
void
Sys_thread_schedule_frame::old_param (L4_sched_param op)
{ _eax = op.raw(); }

IMPLEMENT inline
Unsigned64
Sys_thread_schedule_frame::time() const
{ return (Unsigned64) _edx << 32 | (Unsigned64) _ecx; }

IMPLEMENT inline
void
Sys_thread_schedule_frame::time (Unsigned64 t)
{
  _ecx = t;
  _edx = t >> 32;
}

//---------------------------------------------------------------------------
// Sys-unmap frame for IA32 (v2 and x0)
//

IMPLEMENT inline
L4_fpage Sys_unmap_frame::fpage() const
{ return L4_fpage(_eax); }

IMPLEMENT inline
Mword Sys_unmap_frame::map_mask() const
{ return _ecx; }

IMPLEMENT inline
bool Sys_unmap_frame::downgrade() const
{ return !(_ecx & 2); }

IMPLEMENT inline
bool Sys_unmap_frame::no_unmap() const
{ return _ecx & 4; }

IMPLEMENT inline
bool Sys_unmap_frame::reset_references() const
{ return _ecx & 8; }

IMPLEMENT inline
Task_num Sys_unmap_frame::restricted() const
{ return (_ecx & 0x7ff00) >> 8; }

IMPLEMENT inline
bool Sys_unmap_frame::self_unmap() const
{ return _ecx & 0x80000000; }

IMPLEMENT inline
void
Sys_unmap_frame::ret (Mword status)
{
  // keep the fpage part of EAX intact
  _eax = (_eax & ~L4_fpage::Status_mask) | status;
}

//---------------------------------------------------------------------------
// Sys-task-new frame for IA32 (v2 and x0)
//

IMPLEMENT inline 
Mword Sys_task_new_frame::trigger_exception() const 
{ return _eax & (1 << 30); }

IMPLEMENT inline 
Mword Sys_task_new_frame::alien() const 
{ return _eax & (1 << 31); }

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{ return _eax & ~(3 << 30); }

IMPLEMENT inline 
Mword Sys_task_new_frame::sp() const
{ return _ecx; }

IMPLEMENT inline 
Mword Sys_task_new_frame::ip() const
{ return _edx; }
 
IMPLEMENT inline
Mword Sys_task_new_frame::has_pager() const
{ return _ebx; }

IMPLEMENTATION [(ia32 || ux) && caps]:

IMPLEMENT inline
Mword Sys_task_new_frame::extra_args() const
{ return _eax & (1 << 29); }

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [(ia32 | ux) & caps & utcb]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_task_new_frame::cap_handler(const Utcb* utcb) const
{
  if (! extra_args())
    return L4_uid::Invalid;

  return utcb->values[2];
}



//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [(ia32|ux) & !(caps & utcb)]:

#include "utcb.h"

// If UTCBs are not available, changing or requesting the
// task-capability fault handler is not supported on IA32.

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_task_new_frame::cap_handler(const Utcb* /*utcb*/) const
{ return L4_uid::Invalid; }

//////////////////////////////////////////////////////////////////////
IMPLEMENTATION [(ia32 || ux) && utcb && caps]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb* utcb) const
{ 
  if (! extra_args())
    return L4_quota_desc(0);

  return L4_quota_desc(utcb->values[4]); 
}

//////////////////////////////////////////////////////////////////////
IMPLEMENTATION [(ia32|ux) && (!utcb || !caps)]:

IMPLEMENT inline
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb*) const
{ return L4_quota_desc(0); }

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [{ia32,ux}-v2]:

IMPLEMENT inline 
void Sys_ipc_frame::rcv_src(L4_uid id) 
{
  _esi = id.raw();
  _edi = (id.raw() >> 32);
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_src() const
{ return L4_uid(((Unsigned64)_edi << 32) | (Unsigned64)_esi); }

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dst() const
{ return L4_uid((Unsigned64)_esi | ((Unsigned64)_edi << 32)); }

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word(unsigned index) const
{
  switch(index)
    {
    case 0:
      return _edx;
    case 1:
      return _ebx;
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
      _edx = value;
      break;
    case 1:
      _ebx = value;
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
  Unsigned32 tmp_edx = _edx, tmp_ebx = _ebx;
  to->_edx = tmp_edx;
  to->_ebx = tmp_ebx;
}

//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline
L4_uid Sys_id_nearest_frame::dst() const
{ return L4_uid((Unsigned64)_esi | ((Unsigned64)_edi << 32)); }

IMPLEMENT inline
void Sys_id_nearest_frame::nearest(L4_uid id)
{
  _esi = id.raw();
  _edi = (id.raw() >> 32);
}

//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::preempter() const
{ return L4_uid((Unsigned64)_ebx | ((Unsigned64)_ebp << 32)); }

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::pager() const
{ return L4_uid((Unsigned64)_esi | ((Unsigned64)_edi << 32)); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_preempter(L4_uid id)
{
  _ebx = id.raw();
  _ebp = id.raw() >> 32;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager(L4_uid id)
{
  _esi = id.raw();
  _edi = id.raw() >> 32;
}


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline
L4_uid
Sys_thread_switch_frame::dst() const
{ return L4_uid ((Unsigned64) _esi); }

//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::preempter() const 
{ return L4_uid ((Unsigned64) _ebx | (Unsigned64) _ebp << 32); }

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::dst() const
{ return L4_uid ((Unsigned64) _esi | (Unsigned64) _edi << 32); }

IMPLEMENT inline 
void
Sys_thread_schedule_frame::old_preempter (L4_uid id)
{
  _ebx = id.raw();
  _ebp = id.raw() >> 32;
}

IMPLEMENT inline
void 
Sys_thread_schedule_frame::partner (L4_uid id)
{
  _esi = id.raw();
  _edi = id.raw() >> 32;
}

//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline
L4_uid Sys_task_new_frame::new_chief() const
{ return L4_uid((Unsigned64)(_eax & ~(1 << 31))); }

IMPLEMENT inline
L4_uid Sys_task_new_frame::pager() const
{ return L4_uid((Unsigned64)_ebx | ((Unsigned64)_ebp << 32)); }

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dst() const
{ return L4_uid((Unsigned64)_esi | ((Unsigned64)_edi << 32)); }

IMPLEMENT inline
void Sys_task_new_frame::new_taskid(L4_uid id)
{
  _esi = id.raw();
  _edi = id.raw() >> 32;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ia32-x0,ux-x0]:

IMPLEMENT inline
void Sys_ipc_frame::rcv_src(L4_uid id) 
{ _esi = id.raw(); }

IMPLEMENT inline
L4_uid Sys_ipc_frame::rcv_src() const 
{ return L4_uid(_esi); }

IMPLEMENT inline
L4_uid Sys_ipc_frame::snd_dst() const
{ return L4_uid(_esi); }

IMPLEMENT inline
Mword Sys_ipc_frame::msg_word(unsigned index) const
{
  switch(index)
    {
    case 0:
      return _edx;
    case 1:
      return _ebx;
    case 2:
      return _edi;
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
      _edx = value;
      break;
    case 1:
      _ebx = value;
      break;
    case 2:
      _edi = value;
      break;
    default:
      break;
    }
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{ return 3; }

IMPLEMENT inline
void Sys_ipc_frame::copy_msg(Sys_ipc_frame *to) const
{
  to->_edx = _edx;
  to->_ebx = _ebx;
  to->_edi = _edi;
}

//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline
L4_uid Sys_id_nearest_frame::dst() const
{ return L4_uid(_esi); }

IMPLEMENT inline
void Sys_id_nearest_frame::nearest(L4_uid id)
{ _esi = id.raw(); }

//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::preempter() const
{ return L4_uid(_ebx); }

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::pager() const
{ return L4_uid(_esi); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_preempter(L4_uid id)
{ _ebx = id.raw(); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager(L4_uid id)
{ _esi = id.raw(); }


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline
L4_uid
Sys_thread_switch_frame::dst() const
{ return L4_uid (_esi); }

//Sys_thread_schedule_frame::----------------------------------------------


IMPLEMENT inline
L4_uid Sys_thread_schedule_frame::preempter() const
{ return L4_uid(_ebx); }

IMPLEMENT inline
L4_uid Sys_thread_schedule_frame::dst() const
{ return L4_uid(_esi); }

IMPLEMENT inline
void Sys_thread_schedule_frame::old_preempter(L4_uid id)
{ _ebx = id.raw(); }

IMPLEMENT inline
void Sys_thread_schedule_frame::partner(L4_uid id)
{ _esi = id.raw(); }


//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline
L4_uid Sys_task_new_frame::new_chief() const
{ return L4_uid(_eax & ~(1 << 31)); }

IMPLEMENT inline
L4_uid Sys_task_new_frame::pager() const
{ return L4_uid(_ebx); }

IMPLEMENT inline
L4_uid Sys_task_new_frame::dst() const
{ return L4_uid(_esi); }

IMPLEMENT inline
void Sys_task_new_frame::new_taskid(L4_uid id)
{ _esi = id.raw(); }


IMPLEMENTATION[{ia32,ux}-v2-lipc]:

//Sys_ipc_frame::----------------------------------------------------
IMPLEMENT inline
void Sys_ipc_frame::snd_utcb(Mword local_id)
{ _ecx = local_id; }

IMPLEMENT inline
void Sys_ipc_frame::timeout(Mword t)
{ _ecx = t; }

IMPLEMENTATION[pl0_hack]:

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::command() const
{ return _eax; }

IMPLEMENT inline
L4_uid Sys_thread_privctrl_frame::dst() const
{ return L4_uid(_edx, 0); }

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::entry_func() const
{ return _edx; }

IMPLEMENT inline
void Sys_thread_privctrl_frame::ret_val(Mword v)
{ _eax = v; }
