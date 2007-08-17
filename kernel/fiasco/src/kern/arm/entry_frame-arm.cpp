/*
 * Fiasco Kernel-Entry Frame-Layout Code for ARM
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
IMPLEMENTATION [arm]:

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
Mword Sys_ipc_frame::next_period() const
{ return false; }

IMPLEMENT inline
void Sys_ipc_frame::rcv_src( L4_uid const &id )
{ r[0] = id.raw(); }

IMPLEMENT inline
L4_uid Sys_ipc_frame::rcv_src() const
{ return L4_uid(r[0]); }

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
L4_timeout_pair Sys_ipc_frame::timeout() const
{ return L4_timeout_pair(r[3]); }
  

IMPLEMENT inline
L4_msg_tag Sys_ipc_frame::tag() const
{ return L4_msg_tag(r[4]); }

IMPLEMENT inline
void Sys_ipc_frame::tag(L4_msg_tag const &tag)
{ r[4] = tag.raw(); }

IMPLEMENT inline
Mword Sys_ipc_frame::msg_word( unsigned index ) const
{
  if(index < 2)
    return r[index+5];
  else
    return 0;
}

IMPLEMENT inline
void Sys_ipc_frame::set_msg_word( unsigned index, Mword value )
{
  if(index < 2)
    r[index+5] = value;
}

IMPLEMENT inline
L4_rcv_desc Sys_ipc_frame::rcv_desc() const
{ return r[2]; }

IMPLEMENT inline
void Sys_ipc_frame::rcv_desc( L4_rcv_desc d )
{ r[2] = d.raw(); }

IMPLEMENT inline
L4_msgdope Sys_ipc_frame::msg_dope() const
{ return L4_msgdope(r[1]); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error( Mword e )
{ 
  // only type-punning via char* is permitted
  char *l = reinterpret_cast<char*>(&r[1]);
  reinterpret_cast<L4_msgdope*>(l)->error(e);
}

IMPLEMENT inline
unsigned Sys_ipc_frame::num_reg_words()
{ return 2; } /* This should be 9 but is 2 to make long-IPC compatible with the
               * current scheme (2 dwords in IPC) */

IMPLEMENT inline
unsigned Sys_ipc_frame::num_snd_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
unsigned Sys_ipc_frame::num_rcv_reg_words()
{ return num_reg_words(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope( L4_msgdope d )
{ r[1] = d.raw(); }

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_combine( Ipc_err d )
{
  L4_msgdope m(r[1]);
  m.combine(d);
  r[1] = m.raw();
}

IMPLEMENT inline
void Sys_ipc_frame::copy_msg( Sys_ipc_frame *to ) const
{
  for(unsigned x = 5; x<=6; ++x )
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
void Sys_id_nearest_frame::nearest( L4_uid const &id )
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
bool Sys_unmap_frame::no_unmap() const
{ return r[1] & 4; }

IMPLEMENT inline
bool Sys_unmap_frame::reset_references() const
{ return 0; } 

IMPLEMENT inline
void
Sys_unmap_frame::ret (Mword status)
{
  // keep the fpage part of EAX intact
  r[0] = (r[0] & ~L4_fpage::Status_mask) | status;
}

IMPLEMENT inline
bool Sys_unmap_frame::self_unmap() const
{ return r[1] & 0x80000000; }

IMPLEMENT inline
Task_num Sys_unmap_frame::restricted() const
{ return (r[1] & 0x7ff00) >> 8; }

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
Mword Sys_ex_regs_frame::trigger_exception() const
{ return r[0] & (1 << 28); }

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
void Sys_ex_regs_frame::old_preempter( L4_uid const &id )
{ r[5] = id.raw(); }

IMPLEMENT inline
void Sys_ex_regs_frame::old_pager( L4_uid const &id )
{ r[3] = id.raw(); }

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [arm-caps]:

IMPLEMENT inline 
L4_uid Sys_ex_regs_frame::cap_handler(const Utcb* /*utcb*/) const
{
  if (! (r[0] & (1 << 27)))
    return L4_uid::Invalid;

  return r[6];
}

IMPLEMENT inline
void Sys_ex_regs_frame::old_cap_handler(L4_uid const &id, Utcb* /*utcb*/)
{
  r[6] = id.raw();
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION [arm-!caps]:


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

IMPLEMENTATION [arm]:
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

//--Sys_ulock_frame----------------------------------------------------------

IMPLEMENT inline
Sys_u_lock_frame::Op Sys_u_lock_frame::op() const
{ return (Op)r[0]; }

IMPLEMENT inline
unsigned long Sys_u_lock_frame::lock() const
{ return r[1]; }

IMPLEMENT inline
void Sys_u_lock_frame::result(unsigned long res)
{ r[0] = res; }

IMPLEMENT inline
L4_timeout Sys_u_lock_frame::timeout() const
{ return L4_timeout(r[3]); }

IMPLEMENT inline
L4_semaphore *Sys_u_lock_frame::semaphore() const
{ return (L4_semaphore*)r[2]; }

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
void Sys_thread_schedule_frame::old_preempter( L4_uid const &id )
{ r[2] = id.raw(); }

IMPLEMENT inline
void Sys_thread_schedule_frame::partner( L4_uid const &id )
{ r[1] = id.raw(); }

//Sys_task_new_frame::-------------------------------------------------------

IMPLEMENT inline
Mword Sys_task_new_frame::trigger_exception() const
{ return r[1] & (1 << 30); }

IMPLEMENT inline
Mword Sys_task_new_frame::alien() const
{ return r[1] & (1 << 31); }

IMPLEMENT inline
Mword Sys_task_new_frame::mcp() const
{ return r[1] & ~(3 << 30); }

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
void Sys_task_new_frame::new_taskid( L4_uid const &id )
{ r[0] = id.raw(); }

IMPLEMENT inline
Mword Sys_task_new_frame::extra_args() const
{ return r[0] & (1 << 29); }

//////////////////////////////////////////////////////////////////////
IMPLEMENTATION[caps]:

#include "utcb.h"

IMPLEMENT inline
L4_uid Sys_task_new_frame::cap_handler(const Utcb* utcb) const
{
  if (!extra_args())
    return L4_uid::Invalid;
  return utcb->values[1];
}

IMPLEMENT inline
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb* /*utcb*/) const
{
  if (!extra_args())
    return L4_quota_desc(0);
  return L4_quota_desc(utcb->values[2]);
}

//////////////////////////////////////////////////////////////////////
IMPLEMENTATION [!caps]:

#include "utcb.h"

IMPLEMENT inline NEEDS["utcb.h"]
L4_uid Sys_task_new_frame::cap_handler(const Utcb* /*utcb*/) const
{ return L4_uid::Invalid; }

IMPLEMENT inline
L4_quota_desc Sys_task_new_frame::quota_descriptor(const Utcb* /*utcb*/) const
{ return L4_quota_desc(0); }


