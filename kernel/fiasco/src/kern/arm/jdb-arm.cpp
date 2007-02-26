INTERFACE [arm]:

EXTENSION class Jdb_entry_frame
{
public:
  Unsigned32 r[15];
  Unsigned32 spsr;
  Unsigned32 cpsr;
  Unsigned32 ksp;
  Unsigned32 pc;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "globals.h"
#include "kmem_alloc.h"
#include "kmem_space.h"
#include "space.h"
#include "mem_layout.h"
#include "mem_unit.h"
#include "static_init.h"


STATIC_INITIALIZE_P(Jdb, JDB_INIT_PRIO);

PUBLIC static
FIASCO_INIT FIASCO_NOINLINE void
Jdb::init()
{
  static Jdb_handler enter(at_jdb_enter);
  static Jdb_handler leave(at_jdb_leave);

  Jdb::jdb_enter.add(&enter);
  Jdb::jdb_leave.add(&leave);
}

IMPLEMENT inline
bool Jdb_entry_frame::from_user() const
{ return (spsr & 0x1f) == 0x10; }

IMPLEMENT inline
Address Jdb_entry_frame::ip() const
{ return pc; }

IMPLEMENT 
Context *Jdb::current_context()
{
  return context_of((void*)entry_frame->ksp);
}

PUBLIC static
int
Jdb::peek_addr_task(Address virt, Task_num task, Address *result)
{
  return peek_mword_task(virt, task, result);
}

PUBLIC static
int
Jdb::peek_mword_task(Address virt, Task_num task, Mword *result)
{
  // align
  virt &= ~0x03;

  Address phys;

  if (task == 0 || task == 2)
    {
      if (Mem_layout::in_kernel(virt))
	{
	  Address size;
	  Page::Attribs a;
	  phys = Kmem_space::kdir()->lookup((void *)virt, &size, &a).get_unsigned();
	  if (phys == (Address)-1)
	    return -1;
	}
      else
	phys = virt;
    }
  else
    {
      Space *s = Space::id_lookup(task);
      if (!s)
	return -1;

      phys = s->virt_to_phys_s0((void *)virt);

      if (phys == (Address)-1)
	return -1;
    }

  phys = Mem_layout::phys_to_pmem(phys);
  if (phys == (Address)-1)
    return -1;
  
  *result = *((Mword*)phys);
  return 1;
}

PUBLIC static
int
Jdb::is_adapter_memory(Address, Task_num)
{
  return 0;
}

PUBLIC static
void
Jdb::poke_mword_task(Address virt, Task_num, Mword val)
{
  if (virt >= Mem_layout::User_max)
    *(Mword *)virt = val;
  else
    puts("POKE mword for user unimplemented");
}

PUBLIC static
int
Jdb::peek_task(Address addr, Task_num task)
{
  Mword dummy;
  return peek_mword_task(addr, task, &dummy);
}


PRIVATE static
void
Jdb::at_jdb_enter()
{
  Mem_unit::clean_dcache();
}

PRIVATE static
void
Jdb::at_jdb_leave()
{
  Mem_unit::flush_cache();
}
