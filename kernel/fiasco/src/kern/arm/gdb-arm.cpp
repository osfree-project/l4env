IMPLEMENTATION [arm]:
#if 0
IMPLEMENT
Gdb_serv::Tlb Gdb_s::lookup( unsigned long v_addr ) const
{
  if (v_addr >=0xf0000000)
    return Tlb( 0xf0000000, 0xf0000000 );
  else
    return Tlb();
}
#endif

static inline Gdb_s::Tid uid2tid( bool user, L4_uid id ) 
{
  if (id.is_invalid())
    return -1UL;

  return (id.gthread() << 1) + 2 + user;
}
static inline L4_uid tid2uid( Gdb_s::Tid tid ) 
{
  L4_uid id;
  if (!tid)
    return id;

  tid = (tid-2) >> 1;
  
  id.task(tid/id.threads_per_task());
  id.lthread(tid%id.threads_per_task());

  return id;
}

IMPLEMENT
unsigned char *Gdb_s::get_register( unsigned index, unsigned &size )
{
  static unsigned char fp_dummy[12];

  *(unsigned long*)fp_dummy = 0;

  if (current_tid != get_current_thread())
    {
      Thread *t = Thread::lookup(tid2uid(current_tid));
      if (t)
	{
	  if (current_tid & 1) 
	    { // user thread
	      unsigned long *dummy = (unsigned long*)fp_dummy;
	      size = 4;
	      switch (index)
		{
		case 0:  case 1:  case 2:  case 3:
		case 4:  case 5:  case 6:  case 7: 
		case 8:  case 9:  case 10: case 11:
		case 12: 
		  *dummy = t->regs()->r[index];
		  return fp_dummy;
		case 13:
		  *dummy = t->regs()->sp();
		  return fp_dummy;
		case 14:
		  *dummy = t->regs()->ulr;
		  return fp_dummy;
		case 15:
		  *dummy = t->regs()->ip();
		  return fp_dummy;
		case 25:
		  *dummy = t->regs()->psr;
		  return fp_dummy;
		case 24:
		  return fp_dummy;
		default:
		  size = 12;
		  return fp_dummy;
		}
	    }
	  else
	    { // kernel thread
	      switch (index)
		{
		case 15: // pc
		  size = 4;
		  *(Mword*)fp_dummy = *(t->get_kernel_sp()) + 4;
		  return fp_dummy;
		  //	      return (unsigned char*)t->get_kernel_sp();
		case 13: // sp
		  size = 4;
		  *(Mword**)fp_dummy = t->get_kernel_sp()+2;
		  return fp_dummy;
		case 11: // fp
		  size = 4;
		  return (unsigned char*)(t->get_kernel_sp()+1);

		case 0:   case 1:   case 2:
		case 3:   case 4:   case 5:
		case 6:   case 7:   case 8:
		case 9:   case 10:  
		case 12:  case 14:  case 24:
		case 25:
		  size = 4;
		  return fp_dummy;
		case 16:   case 17:   case 18:
		case 19:   case 20:   case 21:
		case 22:   case 23:
		  size = 12;
		  return fp_dummy;
		default: 
		  return 0;
		}
	    }
	}
      else
	{
	  if (index < 16 || (index >=24))
	    size = 4;
	  else 
	    size = 12;

	  return fp_dummy;
	}
    }
  
  if (index < 15)
    {
      size = 4;
      return (unsigned char*)&Gdb::entry_frame->r[index];
    }
  else if (index == 15)
    {
      size = 4;
      return (unsigned char*)&Gdb::entry_frame->pc;
    }
  else if (index == 25)
    {
      size = 4;
      return (unsigned char*)&Gdb::entry_frame->cpsr;
    }
  else if (index > 15 && index <=23)
    {
      size = 12;
      return fp_dummy;
    }
  else if (index == 24)
    {
      size = 4;
      return fp_dummy;
    }
  return 0;
}

IMPLEMENT
unsigned Gdb_s::pc_index() const
{
  return 15;
}

IMPLEMENT
bool Gdb_s::push_register( unsigned index ) const
{
  switch (index)
    {
    case 11: // FP
    case 13: // SP
    case 14: // LR
    case 15: // PC
      return true;
    default: 
      return false;
    }
}

IMPLEMENT
unsigned Gdb_s::num_registers() const
{
  return 26;
}

IMPLEMENT
Jdb_entry_frame *Gdb_s::get_entry_frame()
{
  extern Jdb_entry_frame _kdebug_stack_top[];
  return _kdebug_stack_top -1;
}

#if 0
IMPLEMENT
bool Gdb_s::thread_alive( unsigned long tid ) const
{
  return 0;
}
#endif


IMPLEMENTATION [arm]: //-thread

#include "thread.h"
#include "space.h"
#include "kmem.h"
#include "helping_lock.h"

static Thread *cs;
static bool cs_user;

PUBLIC 
Gdb_s::Tid Gdb_s::first_thread() const
{
  cs_user = false;
  cs = 0;
  if (!Helping_lock::threading_system_active)
    return -1UL;
  
  cs = Thread::lookup( L4_uid(0,0) );
  if (!cs)
    return -1UL;
  else
    return uid2tid( false, cs->id() );
}

PUBLIC
Gdb_s::Tid Gdb_s::next_thread() const
{
  if (!cs)
    return -1UL;

  if (!cs_user)
    return uid2tid( cs_user=true, cs->id());

  cs = cs->next_present();
  
  if (!cs || cs->id().is_nil())
    return -1UL;

  return uid2tid(cs_user=false, cs->id());
}

PUBLIC
bool Gdb_s::thread_alive( Tid tid ) const
{
  if (!tid)
    return false;

  Thread *t = Thread::lookup( tid2uid(tid) );
  if (!t)
    return false;

  if (!lookup((unsigned long)t).valid())
    return false;

  if (t->state() == 0 || (t->state() & Thread_dead))
    return false;

  return true;
}

PUBLIC
char const *Gdb_s::thread_extra_info( Tid tid ) const
{
  static char buf[128];
  L4_uid th = tid2uid(tid);
  Thread *t = Thread::lookup(th);
  snprintf(buf,sizeof(buf)-1,"%x.%x[%c]: [%08x]",
           th.task(), th.lthread(), tid & 1 ? 'u' : 'k',
	   t->state());
  return buf;
}

PUBLIC
Gdb_s::Tid Gdb_s::get_current_thread() const
{
  if (Kmem::is_tcb_page_fault(Gdb::entry_frame->ksp,0))
    {
      bool user = ((Gdb::entry_frame->spsr & 0x1f) == 0x10);
      return uid2tid( user, 
	Thread::lookup(context_of((void*)Gdb::entry_frame->ksp))->id());
    }
  else
    return 0; //uid2tid(L4_uid(0,0));
}

IMPLEMENT
Gdb_serv::Tlb Gdb_s::lookup( unsigned long v_addr ) const
{
  Space *s = 0;
  if (v_addr >=0xf0000000)
    s = Space::kernel_space();
  else
    {
      Thread *t = Thread::lookup(tid2uid(current_tid));
      if (!t)
	return Tlb();

      s = t->space();
    }
  
  if (!s)
    return Tlb();

  Address size;
  Address phys;

  if (!s->v_lookup(v_addr, &phys, &size, 0))
    return Tlb();

  if (phys <Kmem::Sdram_phys_base 
      || phys >=Kmem::Sdram_phys_base + 256*1024*1024)
    return Tlb();
  
  unsigned long mask = ~(size-1);
  
  return Tlb( mask, v_addr & mask, (phys & mask) - Mem_layout::Sdram_phys_base + Mem_layout::Map_base ); 
}
