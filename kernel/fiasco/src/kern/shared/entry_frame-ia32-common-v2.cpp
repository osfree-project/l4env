/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE:

#include "types.h"


EXTENSION class Syscall_frame
{
public:
  //protected:
  Unsigned32 ecx;
  Unsigned32 edx;
  Unsigned32 esi;
  Unsigned32 edi;
  Unsigned32 ebx;
  Unsigned32 ebp;
  Unsigned32 eax;
};

EXTENSION class Return_frame
{
public:
  Unsigned32 eip;
  Unsigned16 cs, __csu;
  Unsigned32 eflags;
  Unsigned32 esp;
  Unsigned16 ss, __ssu;

};


IMPLEMENTATION[ia32-common-v2]:


IMPLEMENT inline
Mword Entry_frame::pc() const
{
  return eip;
}

IMPLEMENT inline
void Entry_frame::pc( Mword _pc )
{
  eip = _pc;
}

IMPLEMENT inline
Mword Entry_frame::sp() const
{
  return esp;
}

IMPLEMENT inline
void Entry_frame::sp( Mword _sp )
{
  esp = _sp;
}


IMPLEMENT inline 
void Sys_ipc_frame::rcv_source( L4_uid id ) 
{ 
  esi = id.raw(); edi = (id.raw() >> 32); 
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_source() 
{ 
  return L4_uid(((Unsigned64)edi << 32) | (Unsigned64)esi);
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dest() const
{ 
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) ); 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dest() const
{ 
  return esi; 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{ 
  return esi -1; 
}

IMPLEMENT inline 
void Sys_ipc_frame::snd_desc( Mword w ) 
{ 
  eax =w; 
}

IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{ 
  return eax; 
}

IMPLEMENT inline 
L4_timeout Sys_ipc_frame::timeout() const 
{ 
  return L4_timeout( ecx ); 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word( unsigned index ) const
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
void Sys_ipc_frame::set_msg_word( unsigned index, Mword value )
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
L4_rcv_desc Sys_ipc_frame::rcv_desc() const 
{ 
  return ebp; 
}

IMPLEMENT inline 
void Sys_ipc_frame::rcv_desc( L4_rcv_desc d ) 
{ 
  ebp = d.raw(); 
}

IMPLEMENT inline 
L4_msgdope Sys_ipc_frame::msg_dope() const 
{ 
  return L4_msgdope(eax); 
}

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error( Mword e )
{
  reinterpret_cast<L4_msgdope*>(&eax)->error(e);
}

IMPLEMENT inline
unsigned const Sys_ipc_frame::num_reg_words()
{
  return 2;
}

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope( L4_msgdope d ) 
{ 
  eax = d.raw(); 
}

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope_combine( L4_msgdope d ) 
{ 
  L4_msgdope m(eax);
  m.combine(d);
  eax = m.raw(); 
}

IMPLEMENT inline 
void Sys_ipc_frame::copy_msg( Sys_ipc_frame *to ) const 
{
  // hint for gcc to prevent stall
  Unsigned32 tmp_edx = edx, tmp_ebx = ebx;
  to->edx = tmp_edx;
  to->ebx = tmp_ebx;
}



//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline 
L4_uid Sys_id_nearest_frame::dest() const
{ 
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline 
void Sys_id_nearest_frame::nearest( L4_uid id ) 
{ 
  esi = id.raw(); edi = (id.raw() >> 32); 
}



//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline 
Mword Sys_ex_regs_frame::lthread() const 
{ 
  return eax; 
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
void Sys_ex_regs_frame::old_eflags( Mword oefl ) 
{ 
  eax = oefl; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_sp( Mword osp ) 
{ 
  ecx = osp; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_ip( Mword oip ) 
{ 
  edx = oip; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_preempter( L4_uid id ) 
{
  ebx = id.raw(); ebp = id.raw() >> 32;
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_pager( L4_uid id )
{
  esi = id.raw(); edi = id.raw() >> 32;
}


//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline 
L4_uid Sys_thread_switch_frame::dest() const
{ 
  //warning this is against the spec, where only id.low in esi is significant
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline 
Mword Sys_thread_switch_frame::has_dest() const
{ 
  return esi; 
}


//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline 
L4_sched_param Sys_thread_schedule_frame::param() const
{ 
  return L4_sched_param(eax); 
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::preempter() const 
{
  return L4_uid( (Unsigned64)ebx | ((Unsigned64)ebp << 32) );
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::dest() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_param( L4_sched_param op ) 
{ 
  eax = op.raw(); 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::time( Unsigned64 t ) 
{ 
  ecx = t; edx = t >> 32; 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_preempter( L4_uid id )
{
  ebx = id.raw(); ebp = id.raw() >> 32;
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::partner( L4_uid id )
{
  esi = id.raw(); edi = id.raw() >> 32;
}


//Sys_unmap_frame::---------------------------------------------------

IMPLEMENT inline 
L4_fpage Sys_unmap_frame::fpage() const
{ 
  return L4_fpage( eax ) ; 
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


//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{ 
  return eax;
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::new_chief() const
{ 
  return L4_uid( (Unsigned64)eax ); 
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

IMPLEMENT inline 
L4_uid Sys_task_new_frame::pager() const
{
  return L4_uid( (Unsigned64)ebx | ((Unsigned64)ebp << 32) );    
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dest() const
{
  return L4_uid( (Unsigned64)esi | ((Unsigned64)edi << 32) );    
}

IMPLEMENT inline 
void Sys_task_new_frame::new_taskid( L4_uid id ) 
{
  esi = id.raw(); edi = id.raw() >> 32;
}
