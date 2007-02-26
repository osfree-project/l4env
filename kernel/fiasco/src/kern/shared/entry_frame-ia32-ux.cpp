/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE[ia32,ux]:

#include "types.h"

EXTENSION class Syscall_frame
{
protected:
  Mword ecx;
  Mword edx;
  Mword esi;
  Mword edi;
  Mword ebx;
  Mword ebp;
  Mword eax;
};

EXTENSION class Return_frame
{
private:
  Mword eip;
  Unsigned16 csseg, __csu;
  Mword eflags;
  Mword esp;
  Unsigned16 ssseg, __ssu;
};

IMPLEMENTATION[ia32,ux]:

//---------------------------------------------------------------------------
// basic Entry_frame methods for IA32
// 
IMPLEMENT inline
Address
Return_frame::ip() const
{ return eip; }

IMPLEMENT inline
void
Return_frame::ip(Mword _pc)
{ eip = _pc; }

IMPLEMENT inline
Address
Return_frame::sp() const
{ return esp; }

IMPLEMENT inline
void
Return_frame::sp(Mword _sp)
{ esp = _sp; }

PUBLIC inline
Mword 
Return_frame::flags() const
{ return eflags; }

PUBLIC inline
void
Return_frame::flags(Mword _flags)
{ eflags = _flags; }

PUBLIC inline
Mword
Return_frame::cs() const
{ return csseg; }

PUBLIC inline
void
Return_frame::cs(Mword _cs)
{ csseg = _cs; }

PUBLIC inline
Mword
Return_frame::ss() const
{ return ssseg; }

PUBLIC inline
void
Return_frame::ss(Mword _ss)
{ ssseg = _ss; }

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32,ux]:

//---------------------------------------------------------------------------
// IPC frame methods for IA32 (x0 and v2)
// 
IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dst() const
{
  return esi;
}
 
IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{
  return esi -1;
}
 
IMPLEMENT inline 
void Sys_ipc_frame::snd_desc(Mword w)
{
  eax =w;
}
 
IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{
  return eax;
}

IMPLEMENT inline
Mword Sys_ipc_frame::has_snd() const
{
  return snd_desc().has_snd();
}

IMPLEMENT inline 
L4_timeout Sys_ipc_frame::timeout() const
{
  return L4_timeout(ecx);
}

IMPLEMENT inline
L4_rcv_desc Sys_ipc_frame::rcv_desc() const
{
  return ebp;
}

IMPLEMENT inline
void Sys_ipc_frame::rcv_desc(L4_rcv_desc d)
{
  ebp = d.raw();
}

IMPLEMENT inline
L4_msgdope Sys_ipc_frame::msg_dope() const
{
  return L4_msgdope(eax);
}

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error(Mword e)
{
  reinterpret_cast<L4_msgdope&>(eax).error(e);
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_snd_reg_words()
{
  return num_reg_words();
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_rcv_reg_words()
{
  return num_reg_words();
}

IMPLEMENT inline
void Sys_ipc_frame::msg_dope(L4_msgdope d)
{
  eax = d.raw();
}

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_combine(Ipc_err e)
{
  L4_msgdope m(eax);
  m.combine(e);
  eax = m.raw();
}

//---------------------------------------------------------------------------
// ID-nearest frame methods for IA32 (v2 and x0)
// 
IMPLEMENT inline
void Sys_id_nearest_frame::type(Mword type)
{
  eax = type;
}

//---------------------------------------------------------------------------
// ex-regs frame methods for IA32 (v2 and x0)
// 
IMPLEMENT inline
Task_num Sys_ex_regs_frame::lthread() const
{
  return eax & (1 << 7) - 1;
}

IMPLEMENT inline
Task_num Sys_ex_regs_frame::task() const
{
  return (eax >> 7) & ((1 << 11) - 1);
}

IMPLEMENT inline
Mword Sys_ex_regs_frame::alien() const
{
  return eax & (1 << 29);
}

IMPLEMENT inline
Mword Sys_ex_regs_frame::no_cancel() const
{
  return eax & (1 << 30);
}

IMPLEMENT inline
Mword Sys_ex_regs_frame::sp() const
{
  return ecx;
}

IMPLEMENT inline
Mword Sys_ex_regs_frame::ip() const
{
  return edx;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_eflags(Mword oefl)
{
  eax = oefl;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_sp(Mword osp)
{
  ecx = osp;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_ip(Mword oip)
{
  edx = oip;
}

//---------------------------------------------------------------------------
// thread-switch frame for IA32 (v2 and x0)
//

IMPLEMENT inline
Mword
Sys_thread_switch_frame::id() const
{
  return eax;
}

IMPLEMENT inline
void
Sys_thread_switch_frame::left (Unsigned64 t)
{
  ecx = t;
  edx = t >> 32;
}

IMPLEMENT inline
void
Sys_thread_switch_frame::ret (Mword val)
{
  eax = val;
}

//---------------------------------------------------------------------------
// thread-schedule frame for IA32 (v2 and x0)
//

IMPLEMENT inline
L4_sched_param
Sys_thread_schedule_frame::param() const
{
  return L4_sched_param (eax);
}

IMPLEMENT inline
void
Sys_thread_schedule_frame::old_param (L4_sched_param op)
{
  eax = op.raw();
}

IMPLEMENT inline
Unsigned64
Sys_thread_schedule_frame::time() const
{
  return (Unsigned64) edx << 32 | (Unsigned64) ecx;
}

IMPLEMENT inline
void
Sys_thread_schedule_frame::time (Unsigned64 t)
{
  ecx = t;
  edx = t >> 32;
}

//---------------------------------------------------------------------------
// Sys-unmap frame for IA32 (v2 and x0)
//

IMPLEMENT inline
L4_fpage Sys_unmap_frame::fpage() const
{
  return L4_fpage(eax);
}

IMPLEMENT inline
Mword Sys_unmap_frame::map_mask() const
{
  return ecx;
}

IMPLEMENT inline
bool Sys_unmap_frame::downgrade() const
{
  return !(ecx & 2);
}

IMPLEMENT inline
bool Sys_unmap_frame::self_unmap() const
{
  return ecx & 0x80000000;
}

//---------------------------------------------------------------------------
// Sys-task-new frame for IA32 (v2 and x0)
//

IMPLEMENT inline 
Mword Sys_task_new_frame::alien() const 
{
  return eax & (1 << 31);
}

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{
  return eax & ~(1 << 31);
}

IMPLEMENT inline 
Mword Sys_task_new_frame::sp() const
{
  return ecx;
}

IMPLEMENT inline 
Mword Sys_task_new_frame::ip() const
{
  return edx;
}
 
IMPLEMENT inline
Mword Sys_task_new_frame::has_pager() const
{
  return ebx;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-v2]:

IMPLEMENT inline 
void Sys_ipc_frame::rcv_src(L4_uid id) 
{
  esi = id.raw();
  edi = (id.raw() >> 32);
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_src() const
{
  return L4_uid(((Unsigned64)edi << 32) | (Unsigned64)esi);
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dst() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word(unsigned index) const
{
  switch(index)
    {
    case 0:
      return edx;
    case 1:
      return ebx;
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
      edx = value;
      break;
    case 1:
      ebx = value;
      break;
    default:
      break;
    }
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{
  return 2;
}

IMPLEMENT inline
void Sys_ipc_frame::copy_msg(Sys_ipc_frame *to) const
{ 
  // hint for gcc to prevent stall
  Unsigned32 tmp_edx = edx, tmp_ebx = ebx;
  to->edx = tmp_edx;
  to->ebx = tmp_ebx;
}

//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline
L4_uid Sys_id_nearest_frame::dst() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline
void Sys_id_nearest_frame::nearest(L4_uid id)
{
  esi = id.raw();
  edi = (id.raw() >> 32);
}

//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::preempter() const
{
  return L4_uid( (Unsigned64)ebx | ((Unsigned64)ebp << 32) );
}

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::pager() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_preempter(L4_uid id)
{
  ebx = id.raw(); ebp = id.raw() >> 32;
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager(L4_uid id)
{
  esi = id.raw();
  edi = id.raw() >> 32;
}


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline
L4_uid
Sys_thread_switch_frame::dst() const
{
  return L4_uid ((Unsigned64) esi);
}

//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::preempter() const 
{
  return L4_uid ((Unsigned64) ebx | (Unsigned64) ebp << 32);
}

IMPLEMENT inline 
L4_uid
Sys_thread_schedule_frame::dst() const
{
  return L4_uid ((Unsigned64) esi | (Unsigned64) edi << 32);
}

IMPLEMENT inline 
void
Sys_thread_schedule_frame::old_preempter (L4_uid id)
{
  ebx = id.raw();
  ebp = id.raw() >> 32;
}

IMPLEMENT inline
void 
Sys_thread_schedule_frame::partner (L4_uid id)
{
  esi = id.raw();
  edi = id.raw() >> 32;
}

//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline
L4_uid Sys_task_new_frame::new_chief() const
{
  return L4_uid( (Unsigned64)(eax & ~(1 << 31)));
}

IMPLEMENT inline
L4_uid Sys_task_new_frame::pager() const
{
  return L4_uid( (Unsigned64)ebx | ((Unsigned64)ebp << 32) );
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dst() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline
void Sys_task_new_frame::new_taskid(L4_uid id)
{
  esi = id.raw();
  edi = id.raw() >> 32;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ia32-x0,ux-x0]:

IMPLEMENT inline
void Sys_ipc_frame::rcv_src(L4_uid id) 
{
  esi = id.raw();
}

IMPLEMENT inline
L4_uid Sys_ipc_frame::rcv_src() const 
{
  return L4_uid(esi);
}

IMPLEMENT inline
L4_uid Sys_ipc_frame::snd_dst() const
{
  return L4_uid(esi);
}

IMPLEMENT inline
Mword Sys_ipc_frame::msg_word(unsigned index) const
{
  switch(index)
    {
    case 0:
      return edx;
    case 1:
      return ebx;
    case 2:
      return edi;
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
      edx = value;
      break;
    case 1:
      ebx = value;
      break;
    case 2:
      edi = value;
      break;
    default:
      break;
    }
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{
  return 3;
}

IMPLEMENT inline
void Sys_ipc_frame::copy_msg(Sys_ipc_frame *to) const
{
  to->edx = edx;
  to->ebx = ebx;
  to->edi = edi;
}

//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline
L4_uid Sys_id_nearest_frame::dst() const
{
  return L4_uid(esi);
}

IMPLEMENT inline
void Sys_id_nearest_frame::nearest(L4_uid id)
{
  esi = id.raw();
}

//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::preempter() const
{
  return L4_uid(ebx);
}

IMPLEMENT inline
L4_uid Sys_ex_regs_frame::pager() const
{
  return L4_uid(esi);
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_preempter(L4_uid id)
{
  ebx = id.raw();
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager(L4_uid id)
{
  esi = id.raw();
}


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline
L4_uid
Sys_thread_switch_frame::dst() const
{ 
  return L4_uid (esi);
}

//Sys_thread_schedule_frame::----------------------------------------------


IMPLEMENT inline
L4_uid Sys_thread_schedule_frame::preempter() const
{ return L4_uid(ebx); }

IMPLEMENT inline
L4_uid Sys_thread_schedule_frame::dst() const
{
  return L4_uid(esi);
}

IMPLEMENT inline
void Sys_thread_schedule_frame::old_preempter(L4_uid id)
{
  ebx = id.raw();
}

IMPLEMENT inline
void Sys_thread_schedule_frame::partner(L4_uid id)
{
  esi = id.raw();
}


//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline
L4_uid Sys_task_new_frame::new_chief() const
{
  return L4_uid(eax & ~(1 << 31));
}

IMPLEMENT inline
L4_uid Sys_task_new_frame::pager() const
{
  return L4_uid(ebx);
}

IMPLEMENT inline
L4_uid Sys_task_new_frame::dst() const
{
  return L4_uid(esi);
}

IMPLEMENT inline
void Sys_task_new_frame::new_taskid(L4_uid id)
{
  esi = id.raw();
}


IMPLEMENTATION[{ia32,ux}-v2-lipc]:

//Sys_ipc_frame::----------------------------------------------------
IMPLEMENT inline
void Sys_ipc_frame::snd_utcb(Mword local_id)
{
  ecx = local_id;
}

IMPLEMENT inline
void Sys_ipc_frame::timeout(Mword t)
{
  ecx = t;
}

IMPLEMENTATION[pl0_hack]:

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::command() const
{
  return eax;
}

IMPLEMENT inline
L4_uid Sys_thread_privctrl_frame::dst() const
{
  return L4_uid(edx,0);
}

IMPLEMENT inline
Mword Sys_thread_privctrl_frame::entry_func() const
{
  return edx;
}

IMPLEMENT inline
void Sys_thread_privctrl_frame::ret_val(Mword v)
{
  eax = v;
}
