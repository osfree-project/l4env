INTERFACE:

#include "l4_types.h"
#include "space.h"
#include "helping_lock.h"

class slab_cache_anon;

class Task : public Space
{
private:
  void free_utcb_pagetable ();
  void host_init (Task_num);
  void map_tbuf();

public:
  ~Task();
  
  Address map_kip();

  Utcb *alloc_utcb(unsigned thread);
  void free_utcb(unsigned thread);
  Address user_utcb(unsigned thread);
};

INTERFACE [utcb]:

EXTENSION class Task
{
private:
  Helping_lock _task_lock;
  enum { Utcbs_per_page = Config::PAGE_SIZE / sizeof(Utcb) };
  void map_utcb_ptr_page();

  class Utcb_page
  {
  public:
    Utcb_page() : _val(0) {}

    void *page() const 
    { return (void*)(_val & ~(Config::PAGE_SIZE-1)); }

    void set_page(void *p)
    { _val = (Mword)p; }

    unsigned ref_cnt() const
    { return _val & (Config::PAGE_SIZE-1); }

    void inc_ref_cnt()
    { ++(char &)_val; }

    void dec_ref_cnt()
    { --(char &)_val; }

    Utcb *utcb(unsigned thread) const
    { 
      return page() 
	? (Utcb*)((char*)page() + (thread % Utcbs_per_page) * sizeof(Utcb))
	: 0; 
    }

  private:
    Mword _val;
  };


  Utcb_page _utcb_pages[(L4_uid::Max_threads_per_task + Utcbs_per_page-1) 
    / Utcbs_per_page];
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
#include "ram_quota.h"

PUBLIC inline
Task::Task (Ram_quota *q, Task_num no)
    : Space (q, no)
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



PRIVATE inline NEEDS["kmem_slab_simple.h"]
void *
Task::operator new (size_t size, void *p)
{
  (void)size;
  assert (size == sizeof (Task));
  return p;
}

PUBLIC inline NEEDS["kmem_slab_simple.h"]
void 
Task::operator delete (void *ptr)
{
  if (ptr)
    allocator()->q_free(reinterpret_cast<Task*>(ptr)->ram_quota(), ptr);
}

PUBLIC static
Task *
Task::create(Ram_quota *quota, Task_num num)
{
  if (void *t = allocator()->q_alloc(quota))
    {
      Task *a = new (t) Task(quota, num);
      if (a->valid())
	return a;

      delete a;
    }

  return 0;
}

PUBLIC inline
bool
Task::valid() const
{ return mem_space()->valid(); }

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
IMPLEMENTATION [!(ia32|ux|amd64)]:

IMPLEMENT inline
Task::~Task()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32|ux|amd64]:
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
	L4_fpage(0, 0, Config::PAGE_SHIFT, Kmem::virt_to_phys (Kip::k())),
	nonull_static_cast<Space*>(this),  // to: space
	L4_fpage(0, 0, Config::PAGE_SHIFT, Mem_layout::Kip_auto_map),
	0).has_error())
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


PRIVATE inline
Task::Utcb_page const &
Task::utcb_page(unsigned thread) const
{ return _utcb_pages[thread/Utcbs_per_page]; }

PRIVATE inline
Task::Utcb_page &
Task::utcb_page(unsigned thread)
{ return _utcb_pages[thread/Utcbs_per_page]; }
  
IMPLEMENT inline
Address 
Task::user_utcb(unsigned thread)
{
  return Mem_layout::V2_utcb_addr + thread * sizeof(Utcb);
}

IMPLEMENT
Utcb *
Task::alloc_utcb(unsigned thread)
{
  Lock_guard<Helping_lock> guard(&_task_lock);
  Utcb_page &utcb_p = utcb_page(thread);

  if (!utcb_p.page())
    {
      void *p = Mapped_allocator::allocator()->q_alloc(ram_quota(), 
	  Config::PAGE_SHIFT);

      if (EXPECT_FALSE(!p))
	return 0;

      Address va = user_utcb(thread) & ~(Config::PAGE_SIZE-1);
      Mem_space::Status res = 
	mem_space()->v_insert(Mem_layout::pmem_to_phys(p), va, 
	    Config::PAGE_SIZE, 
	    Mem_space::Page_writable | Mem_space::Page_user_accessible);

      switch (res)
	{
	case Mem_space::Insert_ok: break;
	case Mem_space::Insert_err_nomem:
	  Mapped_allocator::allocator()->q_free(ram_quota(), 
	      Config::PAGE_SHIFT, p);
	  return 0;
	default:
	  printf("UTCB mapping failed: va=%p, ph=%p, res=%d\n", 
	      (void*)va, p, res);
	  kdb_ke("BUG in utcb allocation");
	  return 0;
	}
      
      utcb_p.set_page(p); // also sets ref_cnt to 0
    }
  
  utcb_p.inc_ref_cnt();
  Utcb *u = utcb_p.utcb(thread);
  memset(u, 0, sizeof(Utcb));
  return u;
}

IMPLEMENT
void
Task::free_utcb(unsigned thread)
{
  Lock_guard<Helping_lock> guard(&_task_lock);
  Utcb_page &utcb_p = utcb_page(thread);

  if (EXPECT_FALSE(!utcb_p.ref_cnt()))
    return;

  utcb_p.dec_ref_cnt();

  if (utcb_p.ref_cnt() == 0)
    {
      Address va = user_utcb(thread) & ~(Config::PAGE_SIZE-1);

      mem_space()->v_delete(va, Config::PAGE_SIZE); 

      Mapped_allocator::allocator()->q_free(ram_quota(), Config::PAGE_SHIFT,
	  utcb_p.page());
      
      utcb_p.set_page(0);
    }
}


/** Allocate space for the UTCBs of all threads in this task.
 *  @ return true on success, false if not enough memory for the UTCBs
 */
PUBLIC
bool
Task::initialize()
{
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
