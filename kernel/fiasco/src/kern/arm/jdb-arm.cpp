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

#include "jdb_tbuf_init.h"
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

  Jdb_tbuf_init::init(0);
}

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

PRIVATE static
Mword *
Jdb::access_mword_task(Address virt, Task_num task)
{
  // align
  virt &= ~0x03;

  Address phys;

  if (task == Config::kernel_taskno || task == Config::sigma0_taskno)
    {
      if (Mem_layout::in_kernel(virt))
	{
	  Pte p = Kmem_space::kdir()->walk((void *)virt, 0, false);
	  if (!p.valid())
	    return 0;

	  phys = p.phys((void*)virt);
	}
      else
	phys = virt;
    }
  else
    {
      Space *s = Space::id_lookup(task);
      if (!s)
	return 0;

      phys = s->mem_space()->virt_to_phys_s0((void *)virt);

      if (phys == (Address)-1)
	return 0;
    }

  unsigned long addr = Mem_layout::phys_to_pmem(phys);
  if (addr == (Address)-1)
    {
      Mem_unit::flush_vdcache();
      Pte pte = Page_table::current()->walk
	((void*)Mem_layout::Jdb_tmp_map_area, 0, false);

      if (pte.phys() != (phys & ~(Config::SUPERPAGE_SIZE-1)))
	pte.set(phys & ~(Config::SUPERPAGE_SIZE-1), Config::SUPERPAGE_SIZE,
	    Page::USER_NO | Page::CACHEABLE, true);

      addr = Mem_layout::Jdb_tmp_map_area + (phys & (Config::SUPERPAGE_SIZE-1));
    }
  
  return (Mword*)addr;
}

PUBLIC static
int
Jdb::peek_mword_task(Address virt, Task_num task, Mword *result)
{
  Mword *mem = access_mword_task(virt, task);
  if (!mem)
    return -1;

  *result = *mem;
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
Jdb::poke_mword_task(Address virt, Task_num task, Mword val)
{
  Mword *mem = access_mword_task(virt, task);
  if (mem)
    *mem = val;
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
  Mem_unit::clean_vdcache();
}

PRIVATE static
void
Jdb::at_jdb_leave()
{
  Mem_unit::flush_vcache();
}

PUBLIC static inline
void
Jdb::enter_getchar()
{}

PUBLIC static inline
void
Jdb::leave_getchar()
{}

PUBLIC static
void
Jdb::write_tsc_s(Signed64 /*tsc*/, char * /*buf*/, int /*maxlen*/, bool /*sign*/)
{}

PUBLIC static
void
Jdb::write_tsc(Signed64 /*tsc*/, char * /*buf*/, int /*maxlen*/, bool /*sign*/)
{}

//---------------------------------------------------------------------------
IMPLEMENT inline
Address_type
Jdb_entry_frame::from_user() const
{ return (spsr & 0x1f) == 0x10 ? ADDR_USER : ADDR_KERNEL; }

IMPLEMENT inline
Address Jdb_entry_frame::ip() const
{ return pc; }

PUBLIC inline
Mword
Jdb_entry_frame::param() const
{ return r[0]; }
