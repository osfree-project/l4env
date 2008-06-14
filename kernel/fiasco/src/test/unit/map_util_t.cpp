IMPLEMENTATION:

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>

using namespace std;

#include "map_util.h"
#include "space.h"
#include "globals.h"
#include "config.h"

#include "boot_info.h"
#include "cpu.h"
#include "config.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "pic.h"
#include "static_init.h"
#include "usermode.h"
#include "vmem_alloc.h"

IMPLEMENTATION:

typedef L4_fpage Test_fpage;

unsigned sigma0_taskno;

class Test_space : public Space
{
public:
  Test_space (Ram_quota *rq, unsigned i)
    : Space (rq, i)
  {}

  void* operator new (size_t s)
  { return malloc (s); }

  void operator delete (void *p)
  { free (p); }
};

STATIC_INITIALIZER_P(init, STARTUP_INIT_PRIO);

static void init()
{
  Usermode::init();
  Boot_info::init();
  Cpu::init();
  Config::init();
  Kmem::init();
  Kip_init::init();
  Kmem_alloc::init();
  Vmem_alloc::init();
  Pic::init();
}

int main()
{
  // 
  // Create tasks
  // 
  Ram_quota rq(0, ~0UL);
  Test_space *sigma0 = new Test_space (&rq, 2);

  sigma0_space = sigma0->mem_space();
  sigma0_taskno = sigma0->id();

  Test_space *server = new Test_space (&rq, 5);
  assert (server);
  Test_space *client = new Test_space (&rq, 6);
  assert (client);
  Test_space *client2 = new Test_space (&rq, 7);
  assert (client2);

  // 
  // Manipulate mappings.
  // 
  Mapdb* mapdb = mapdb_instance();

  Address phys;
  Mword size;
  unsigned page_attribs;
  Mapping *m;
  Mapdb::Frame frame;
  
  // 
  // s0 [0x10000] -> server [0x1000]
  // 
  assert (server->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs) 
	  == false);

  fpage_map (sigma0, 
	     Test_fpage (false, true, Config::PAGE_SHIFT, 0x10000),
	     server,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x1000);

  assert (server->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x10000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  assert (mapdb->lookup (sigma0->id(), 0x10000, 0x10000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // 
  // s0 [0/superpage] -> server [0] -> should map many 4K pages and
  // overmap previous mapping
  // 
  assert (server->mem_space()->v_lookup (0, &phys, &size, &page_attribs)
	  == false);

  fpage_map (sigma0, 
	     L4_fpage (Config::SUPERPAGE_SHIFT, 0),
	     server,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0);

  assert (server->mem_space()->v_lookup (0, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);	// and not SUPERPAGE_SIZE!
  assert (phys == 0);
  assert (page_attribs == Mem_space::Page_user_accessible);

  assert (mapdb->lookup (sigma0->id(), 0, 0, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // previous mapping still there?

  assert (server->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  // Previously, overmapping did not work and was ignored, so the
  // lookup yielded the previously established mapping:
  //   assert (phys == 0x10000);
  //   assert (page_attribs == (Mem_space::Page_writable | Mem_space::Page_user_accessible));
  // Now, the previous mapping should have been overwritten:
  assert (phys == 0x1000);
  assert (page_attribs == Mem_space::Page_user_accessible);

  // mapdb entry -- tree should now contain another mapping 
  // s0 [0x10000] -> server [0x10000]
  assert (mapdb->lookup (sigma0->id(), 0x10000, 0x10000, &m, &frame));
  print_node (m, frame, 0x10000, 0x11000);
  mapdb->free (frame);

  // 
  // Partially unmap superpage s0 [0/superpage]
  // 
  assert (server->mem_space()->v_lookup (0x101000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x101000);
  assert (page_attribs == Mem_space::Page_user_accessible);

  fpage_unmap (sigma0, 
	       Test_fpage (false, true, Config::SUPERPAGE_SHIFT - 2, 0x100000),
	       false, 0, Unmap_full);
  
  assert (mapdb->lookup (sigma0->id(), 0x0, 0x0, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);
  
  assert (! server->mem_space()->v_lookup (0x101000, &phys, &size, 
					   &page_attribs)
	  == true);

  // 
  // s0 [4M/superpage] -> server [8M]
  // 
  assert (server->mem_space()->v_lookup (0x800000, &phys, &size, &page_attribs)
	  == false);

  fpage_map (sigma0, 
	     Test_fpage (false, true, Config::SUPERPAGE_SHIFT, 0x400000),
	     server,
	     L4_fpage (Config::SUPERPAGE_SHIFT, 0x800000),
	     0);

  assert (server->mem_space()->v_lookup (0x800000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::SUPERPAGE_SIZE);
  assert (phys == 0x400000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  assert (mapdb->lookup (sigma0->id(), 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // 
  // server [8M+4K] -> client [8K]
  // 
  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == false);

  fpage_map (server, 
	     Test_fpage (false, true, Config::PAGE_SHIFT, 0x801000),
	     client,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x8000);

  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  // Previously, the 4K submapping is attached to the Sigma0 parent.
  // Not any more.

  assert (mapdb->lookup (sigma0->id(), 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // 
  // Overmap a read-only page.  The writable attribute should not be
  // flushed.
  // 
  fpage_map (server, 
	     Test_fpage (false, false, Config::PAGE_SHIFT, 0x801000),
	     client,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x8000);

  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  


  // 
  // Try selective unmap
  // 
  fpage_map (server, 
	     Test_fpage (false, true, Config::PAGE_SHIFT, 0x801000),
	     client2,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x1000);

  assert (client2->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  

  assert (mapdb->lookup (sigma0->id(), 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  fpage_unmap (server,
	       Test_fpage (false, true, Config::PAGE_SHIFT, 0x801000),
	       false, client2->id(), Unmap_full);

  // Page should have vanished in client2's space, but should still be
  // present in client's space.
  assert (client2->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == false);
  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  

  assert (mapdb->lookup (sigma0->id(), 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  cerr << "... ";


  // 
  // Try some Accessed / Dirty flag unmaps
  // 
  
  // touch page in client
  assert (client->mem_space()->v_insert (0x401000, 0x8000, Config::PAGE_SIZE, 
			    Mem_space::Page_dirty | Mem_space::Page_referenced)
	  == Mem_space::Insert_warn_attrib_upgrade);

  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible 
			   | Mem_space::Page_dirty | Mem_space::Page_referenced));

  // reset dirty from server
  assert (fpage_unmap (server, 
		       Test_fpage (false, true, Config::PAGE_SHIFT, 0x801000),
		       false, 0, Unmap_dirty)
	  == Unmap_dirty);

  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible 
			   | Mem_space::Page_referenced)); // Page_dirty is gone...

  assert (server->mem_space()->v_lookup (0x801000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::SUPERPAGE_SIZE);
  assert (phys == 0x400000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible
			   | Mem_space::Page_dirty)); // ...and went here

  // flush dirty and accessed from server
  assert (fpage_unmap (server, 
		       Test_fpage (false, true, Config::SUPERPAGE_SHIFT, 0x800000),
		       true, 0, Unmap_dirty | Unmap_referenced)
	  == Unmap_dirty | Unmap_referenced);

  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible)); // dirty/ref gone

  assert (server->mem_space()->v_lookup (0x800000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::SUPERPAGE_SIZE);
  assert (phys == 0x400000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible)); // dirty/ref gone

  assert (sigma0->mem_space()->v_lookup (0x400000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::SUPERPAGE_SIZE);
  assert (phys == 0x400000);
  // Be a bit more lax in checking Sigma0's address space:  It does
  // not contain Page_writable / Page_user_accessible flags unless
  // they are faulted in.
  assert (page_attribs & (Mem_space::Page_dirty | Mem_space::Page_referenced));


  // 
  // Delete tasks
  // 
#if 0
  // do not do this because the mapping database would crash if 
  // they has mappings without spaces
  delete server;
  delete client;
  delete client2;
  delete sigma0;
#endif

  cerr << "OK" << endl;

  return 0;
}

static void print_node(Mapping* node, const Mapdb::Frame& frame,
		       Address va_begin = 0, Address va_end = ~0UL)
{
  assert (node);

  size_t size = frame.size();

  for (Mapdb::Iterator i (frame, node, 0, va_begin, va_end); node;)
    {
      for (int d = node->depth(); d != 0; d--)
        cout << ' ';

      cout << setbase(16)
	   << "space=0x"  << (unsigned) (node->space())
	   << " vaddr=0x" << node->page() * size
	   << " size=0x" << size;

      if (Mapping* p = node->parent())
	{
	  cout << " parent=0x" << p->space()
	       << " p.vaddr=0x" << p->page() * size;
	}

      cout << endl;

      node = i;
      if (node)
	{
	  size = i.size();
	  ++i;
	}
    }
  cout << endl;
}

