INTERFACE:

#include "l4_types.h"
#include "space.h"

class slab_cache_anon;

class Task : public Space
{
private:
  void free_utcb_pagetable ();
  void host_init (Task_num);
  void map_tbuf();

public:
  explicit Task (Task_num id);
  ~Task();
  
  Address map_kip();
};

INTERFACE [utcb]:

EXTENSION class Task
{
private:
  void map_utcb_ptr_page();
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "auto_ptr.h"
#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "kmem_slab_simple.h"
#include "l4_types.h"
#include "map_util.h"
#include "mem_layout.h"

IMPLEMENT inline
explicit
Task::Task (Task_num no)
    : Space (no)
{
  host_init (no);

  if (id() == Config::sigma0_taskno)
    return;

  map_tbuf();
}

/** Constructor for special derived classes such as Kernel_task. */
PROTECTED inline
Task::Task(Task_num no, Task_num chief, Mem_space::Dir_type* pdir)
  : Space (no, chief, pdir)
{
}

PRIVATE static
slab_cache_anon* 
Task::allocator ()
{
  static slab_cache_anon* slabs = new Kmem_slab_simple (sizeof (Task), 
							sizeof (Mword),
							"Task");
  return slabs;

  // If Fiasco would kill all tasks even when exiting through the
  // kernel debugger, we could use a deallocating version of the above:
  //
  // static auto_ptr<slab_cache_anon> slabs
  //   (new Kmem_slab_simple (sizeof (Task), sizeof (Mword)))
  // return slabs.get();
}


PUBLIC inline NEEDS["kmem_slab_simple.h"]
void *
Task::operator new (size_t size)
{
  (void)size;
  assert (size == sizeof (Task));

  void *ret;
  check((ret = allocator()->alloc()));
  return ret;
}

PUBLIC inline NEEDS["kmem_slab_simple.h"]
void 
Task::operator delete (void * const ptr)
{
  if (ptr)
    allocator()->free (ptr);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

IMPLEMENT inline
void
Task::host_init (Task_num)
{}

IMPLEMENT inline
void
Task::map_tbuf ()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [!{ia32,ux,amd64}]:

IMPLEMENT inline
Task::~Task()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32,ux,amd64]:
#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "l4_types.h"
#include "map_util.h"
#include "mem_layout.h"

IMPLEMENT 
Address
Task::map_kip ()
{
  Address _kip;
  if (mem_space()->v_lookup(Mem_layout::Kip_auto_map, &_kip, 0, 0))
    {
      if (_kip == Kmem::virt_to_phys (Kip::k()))
	return Mem_layout::Kip_auto_map;
    }
  else if (!mem_map (sigma0_task,                      // from: space
	Kmem::virt_to_phys (Kip::k()),     // from: address
	Config::PAGE_SHIFT,                // from: size
	0, 0,                              // read, map
	nonull_static_cast<Space*>(this),  // to: space
	Mem_layout::Kip_auto_map,          // to: adddress
	Config::PAGE_SHIFT,                // to: size
	0, L4_fpage::Cached).has_error())
    return Mem_layout::Kip_auto_map;

  return ~0UL;
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
  //assert(!(sizeof(Utcb) & (sizeof(Utcb) - 1)));

  if (!Vmem_alloc::local_alloc
           (mem_space(), Mem_layout::V2_utcb_addr, 
	    mem_space()->utcb_size_pages(), 
	    Mem_space::Page_writable | Mem_space::Page_user_accessible))
    {
      kdb_ke("KERNEL: Not enough kmem for utcb area");
      return false;
    }

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
#if 0
  Pd_entry *utcb_pde = _dir.lookup (Mem_layout::V2_utcb_addr);
  (void)utcb_pde;

  assert (utcb_pde && utcb_pde->valid() && !utcb_pde->superpage());
#endif
  Vmem_alloc::local_free (mem_space(), Mem_layout::V2_utcb_addr, 
			  mem_space()->utcb_size_pages());

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
