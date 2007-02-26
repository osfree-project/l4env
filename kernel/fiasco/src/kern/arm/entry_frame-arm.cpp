/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE [arm]:

#include "types.h"

EXTENSION class Syscall_frame
{
public:
  //protected:
  Unsigned32 r[13];
  void dump();
};

EXTENSION class Return_frame
{
public: 
  //protected:
  Unsigned32 psr;
  Unsigned32 _sp;
  Unsigned32 ulr;
  Unsigned32 km_lr;
  Unsigned32 pc;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-x0]:

#include <cstdio>

IMPLEMENT //inline 
void Syscall_frame::dump()
{
#if 1
  for(int i = 0; i < 15; i+=4 )
    printf("R[%2d]: %08x R[%2d]: %08x R[%2d]: %08x R[%2d]: %08x\n",
	   i,r[i],i+1,r[i+1],i+2,r[i+2],i+3,r[i+3]);
#endif
}

IMPLEMENT inline
Mword 
Return_frame::ip() const
{ return Return_frame::pc; }

IMPLEMENT inline
void 
Return_frame::ip(Mword _pc)
{ Return_frame::pc = _pc; }

IMPLEMENT inline
Mword
Return_frame::sp() const
{ return Return_frame::_sp; }

IMPLEMENT inline
void 
Return_frame::sp(Mword sp)
{ Return_frame::_sp = sp; }

//---------------------------------------------------------------------------
IMPLEMENT inline 
void Sys_ipc_frame::rcv_src( L4_uid id ) 
{ r[1] = id.raw(); }

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_src() const
{ return L4_uid(r[1]); }

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dst() const
{ return L4_uid(r[0]); }

IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dst() const
{ return r[0]; }

IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{ return r[0] -1; }

IMPLEMENT inline 
void Sys_ipc_frame::snd_desc( Mword w ) 
{ r[1] =w; }

IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{ return r[1]; }

IMPLEMENT inline
Mword Sys_ipc_frame::has_snd() const
{ return snd_desc().has_snd(); }

IMPLEMENT inline 
L4_timeout Sys_ipc_frame::timeout() const 
{ return L4_timeout( r[3] ); }

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word( unsigned index ) const
{ 
  if(index < 9)
    return r[index+4];
  else
    return 0;
}

IMPLEMENT inline 
void Sys_ipc_frame::set_msg_word( unsigned index, Mword value )
{
  if(index < 9)
    r[index+4] = value;
}

IMPLEMENT inline 
L4_rcv_desc Sys_ipc_frame::rcv_desc() const 
{ return r[2]; }

IMPLEMENT inline 
void Sys_ipc_frame::rcv_desc( L4_rcv_desc d ) 
{ r[2] = d.raw(); }

IMPLEMENT inline 
L4_msgdope Sys_ipc_frame::msg_dope() const 
{ return L4_msgdope(r[0]); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error( Mword e )
{ reinterpret_cast<L4_msgdope*>(&r[0])->error(e); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{ return 3; } /* This should be 9 but is 3 to make long-IPC compatible with the
               * current scheme (3 dwords in IPC) */

IMPLEMENT inline
unsigned Sys_ipc_frame::num_snd_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_rcv_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope( L4_msgdope d ) 
{ r[0] = d.raw(); }

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope_combine( Ipc_err d ) 
{ 
  L4_msgdope m(r[0]);
  m.combine(d);
  r[0] = m.raw(); 
}

IMPLEMENT inline 
void Sys_ipc_frame::copy_msg( Sys_ipc_frame *to ) const 
{
  for(unsigned x = 4; x<13; ++x )
    to->r[x] = r[x];
}

//Entry_id_nearest_data::----------------------------------------------------
IMPLEMENT inline 
L4_uid Sys_id_nearest_frame::dst() const
{ return L4_uid( r[0] ); }

IMPLEMENT inline
void Sys_id_nearest_frame::type( Mword type )
{ r[0] = type; }

IMPLEMENT inline 
void Sys_id_nearest_frame::nearest( L4_uid id ) 
{ r[1] = id.raw(); }

//Sys_unmap_frame::----------------------------------------------------------
IMPLEMENT inline 
L4_fpage Sys_unmap_frame::fpage() const
{ return L4_fpage(r[0]); }

IMPLEMENT inline 
Mword Sys_unmap_frame::map_mask() const
{ return r[1]; }

IMPLEMENT inline 
bool Sys_unmap_frame::downgrade() const
{ return !(r[1] & 2); }

IMPLEMENT inline 
bool Sys_unmap_frame::self_unmap() const
{ return r[1] & 0x80000000; }

//Sys_ex_regs_frame::--------------------------------------------------------
IMPLEMENT inline 
LThread_num Sys_ex_regs_frame::lthread() const 
{ return r[0] & 0x03f; }

IMPLEMENT inline 
Task_num Sys_ex_regs_frame::task() const
{ return (r[0] >> 7) & 0xffff; }

IMPLEMENT inline
Mword Sys_ex_regs_frame::no_cancel() const
{ return r[0] & (1 << 30); }

IMPLEMENT inline
Mword Sys_ex_regs_frame::alien() const
{ return r[0] & (1 << 29); }

IMPLEMENT inline 
Mword Sys_ex_regs_frame::sp() const 
{ return r[2]; }

IMPLEMENT inline 
Mword Sys_ex_regs_frame::ip() const 
{ return r[1]; }

IMPLEMENT inline 
L4_uid Sys_ex_regs_frame::preempter() const
{ return L4_uid( r[5] ); }

IMPLEMENT inline 
L4_uid Sys_ex_regs_frame::pager() const
{ return L4_uid( r[3] ); }

IMPLEMENT inline 
void Sys_ex_regs_frame::old_eflags( Mword oefl ) 
{ r[4] = oefl; }

IMPLEMENT inline 
void Sys_ex_regs_frame::old_sp( Mword osp ) 
{ r[2] = osp; }

IMPLEMENT inline 
void Sys_ex_regs_frame::old_ip( Mword oip ) 
{ r[1] = oip; }

IMPLEMENT inline 
void Sys_ex_regs_frame::old_preempter( L4_uid id ) 
{ r[5] = id.raw(); }

IMPLEMENT inline 
void Sys_ex_regs_frame::old_pager( L4_uid id )
{ r[3] = id.raw(); }

//Sys_thread_switch_frame::--------------------------------------------------

IMPLEMENT inline 
L4_uid
Sys_thread_switch_frame::dst() const
{ 
  //warning this is against the spec, where only id.low in esi is significant
  return L4_uid (r[0]);
}

IMPLEMENT inline				// To be implemented correctly
Mword
Sys_thread_switch_frame::id() const
{
  return 0;
}

IMPLEMENT inline				// To be implemented correctly
void
Sys_thread_switch_frame::left (Unsigned64)
{}
 
IMPLEMENT inline				// To be implemented correctly
void
Sys_thread_switch_frame::ret (Mword)
{}

//Sys_thread_schedule_frame::------------------------------------------------
IMPLEMENT inline
Unsigned64 Sys_thread_schedule_frame::time() const
{ return (Unsigned64) r[4] << 32 | (Unsigned64) r[3]; }

IMPLEMENT inline 
L4_sched_param Sys_thread_schedule_frame::param() const
{ return L4_sched_param(r[0]); }

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::preempter() const 
{ return L4_uid( r[2] ); }

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::dst() const
{ return L4_uid( r[1] ); }

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_param( L4_sched_param op ) 
{ r[0] = op.raw(); }

IMPLEMENT inline 
void Sys_thread_schedule_frame::time( Unsigned64 t ) 
{ r[3] = t; r[4] = t >> 32; }

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_preempter( L4_uid id )
{ r[2] = id.raw(); }

IMPLEMENT inline 
void Sys_thread_schedule_frame::partner( L4_uid id )
{ r[1] = id.raw(); }

//Sys_task_new_frame::-------------------------------------------------------
IMPLEMENT inline 
Mword Sys_task_new_frame::alien() const 
{ return r[1] & (1<<31); }

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{ return r[1] & ~(1 << 31); }

IMPLEMENT inline 
L4_uid Sys_task_new_frame::new_chief() const
{ return L4_uid( r[1] & ~(1<<31)); }

IMPLEMENT inline   
Mword Sys_task_new_frame::sp() const
{ return r[4]; }

IMPLEMENT inline 
Mword Sys_task_new_frame::ip() const
{ return r[3]; }
  
IMPLEMENT inline 
Mword Sys_task_new_frame::has_pager() const
{ return r[2]; }

IMPLEMENT inline 
L4_uid Sys_task_new_frame::pager() const
{ return L4_uid( r[2] ); }

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dst() const
{ return L4_uid( r[0] ); }

IMPLEMENT inline 
void Sys_task_new_frame::new_taskid( L4_uid id ) 
{ r[0] = id.raw(); }

