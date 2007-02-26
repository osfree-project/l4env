INTERFACE:

#include "l4_types.h"
#include "space.h"

class Task : public Space
{
private:
  void free_utcb_pagetable();
  void host_init (Task_num);

public:
  explicit Task (Task_num id);
  ~Task();
};

INTERFACE [utcb]:

EXTENSION class Task
{
private:
  void map_utcb_ptr_page();
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

inline
Task *
current_task()
{  
  return nonull_static_cast<Task*>(current_space());
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

IMPLEMENT inline
void
Task::host_init (Task_num)
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [!{ia32,ux}]:

IMPLEMENT inline
Task::~Task()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [!auto_map_kip]:

IMPLEMENT inline NEEDS[Task::host_init]
Task::Task (Task_num id)
    : Space (id)
{
  host_init (id);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [auto_map_kip]:

#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "l4_types.h"
#include "map_util.h"
#include "mem_layout.h"

IMPLEMENT 
Task::Task (Task_num no)
    : Space (no)
{
  host_init (no);

  if (id() == Config::sigma0_taskno)
    return;

  mem_map (sigma0_space, 
	   Kmem::virt_to_phys (Kip::k()),
	   Config::PAGE_SHIFT,
	   0, 0,
	   nonull_static_cast<Space*>(this), 
	   Mem_layout::Kip_auto_map,
	   Config::PAGE_SHIFT, 0);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [utcb]:

#include "utcb.h"
#include "mem_layout.h"
#include "kdb_ke.h"
#include "paging.h"
#include "vmem_alloc.h"

/** Allocate space for the UTCBs of all threads in this task.
 *  @ return true on success, false if not enough memory for the UTCBs
 */
PUBLIC
bool
Task::initialize()
{
  // make sure sizeof(Utcb) is 2^n so that a UTCB does not cross a page
  // boundary
  // XXX: move that to some kernel startup phase
  assert(!(sizeof(Utcb) & (sizeof(Utcb) - 1)));

  if (!Vmem_alloc::local_alloc
           (static_cast<Space*> (this),
	    Mem_layout::V2_utcb_addr, utcb_size_pages(),
	    (Page_writable | Page_user_accessible)))
    {
      kdb_ke("KERNEL: Not enough kmem for utcb area");
      return false;
    }

  // dummy LIPC KIP location
  // so that the first KIP map to this address space will overwrite it
  set_lipc_kip_pointer((Address) -1);

  
  // For UX, map the UTCB pointer page. For ia32, do nothing
  if (id() != Config::sigma0_taskno)
    map_utcb_ptr_page();

  return true;
}

/**
 * Clean up the task.
 *
 * Unmap all kernel-mapped objects, free the associated Kmem, and
 * finally flush the address space.
 */
PUBLIC
void
Task::cleanup()
{
  Pd_entry *utcb_pde = _dir.lookup (Mem_layout::V2_utcb_addr);
  (void)utcb_pde;

  assert (utcb_pde && utcb_pde->valid() && !utcb_pde->superpage());
  Vmem_alloc::local_free (this, Mem_layout::V2_utcb_addr, utcb_size_pages());

  // if the UTCBs are in the kernel mem (>3GB) also unmap the pagetable
  // free_utcb_pagetable();
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!utcb]:

PUBLIC inline
bool
Task::initialize()
{ return true; }

PUBLIC inline
void
Task::cleanup()
{}
