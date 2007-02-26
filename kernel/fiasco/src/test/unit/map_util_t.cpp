IMPLEMENTATION:

#include <iostream>
#include <iomanip>
#include <cassert>

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
  Test_space (unsigned i)
    : Space (i)
  {}
};

static 
unsigned 
space_number (unsigned s)
{
  return s;
}

STATIC_INITIALIZER_P(init, STARTUP_INIT_PRIO);

static void init()
{
  Usermode::init();
  Boot_info::init();
  Cpu::init();
  Config::init();
  Kmem::init();
  Kmem_alloc::init();
  Kip_init::init();
  Vmem_alloc::init();
  Pic::init();
}

int main()
{
  // 
  // Create tasks
  // 
  sigma0_space = new Test_space (2);
  sigma0_taskno = sigma0_space->id();

  Space *server = new Test_space (5);
  assert (server);
  Space *client = new Test_space (6);
  assert (client);

  // 
  // Manipulate mappings.
  // 
  Mapdb* mapdb = mapdb_instance();

  Address phys;
  Mword size;
  unsigned page_attribs;
  Mapping *m;
  
  // 
  // s0 [0x10000] -> server [0x1000]
  // 
  assert (server->v_lookup (0x1000, &phys, &size, &page_attribs) == false);

  fpage_map (sigma0_space, 
	     Test_fpage (true, true, Config::PAGE_SHIFT, 0x10000),
	     server,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x1000, false);

  assert (server->v_lookup (0x1000, &phys, &size, &page_attribs) == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x10000);
  assert (page_attribs == (Space::Page_writable 
			   | Space::Page_user_accessible));

  m = mapdb->lookup (sigma0_space->id(), 0x10000, 0x10000);
  print_node (m);
  mapdb->free (m);

  // 
  // s0 [0/superpage] -> server [0]  -> should map many 4K pages and skip
  // previos page
  // 
  assert (server->v_lookup (0, &phys, &size, &page_attribs) == false);

  fpage_map (sigma0_space, 
	     L4_fpage (Config::SUPERPAGE_SHIFT, 0),
	     server,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0, false);

  assert (server->v_lookup (0, &phys, &size, &page_attribs) == true);
  assert (size == Config::PAGE_SIZE);	// and not SUPERPAGE_SIZE!
  assert (phys == 0);
  assert (page_attribs == Space::Page_user_accessible);

  m = mapdb->lookup (sigma0_space->id(), 0, 0);
  print_node (m);
  mapdb->free (m);

  // previous mapping still there?
  assert (server->v_lookup (0x1000, &phys, &size, &page_attribs) == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x10000);
  assert (page_attribs == (Space::Page_writable | Space::Page_user_accessible));

  // mapdb entry -- tree should now contain another mapping 
  // s0 [0x10000] -> server [0x10000]
  m = mapdb->lookup (sigma0_space->id(), 0x10000, 0x10000);
  print_node (m);
  mapdb->free (m);

  // 
  // s0 [4M/superpage] -> server [8M]
  // 
  assert (server->v_lookup (0x800000, &phys, &size, &page_attribs) == false);

  fpage_map (sigma0_space, 
	     Test_fpage (true, true, Config::SUPERPAGE_SHIFT, 0x400000),
	     server,
	     L4_fpage (Config::SUPERPAGE_SHIFT, 0x800000),
	     0, false);

  assert (server->v_lookup (0x800000, &phys, &size, &page_attribs) == true);
  assert (size == Config::SUPERPAGE_SIZE);
  assert (phys == 0x400000);
  assert (page_attribs == (Space::Page_writable 
			   | Space::Page_user_accessible));

  m = mapdb->lookup (sigma0_space->id(), 0x400000, 0x400000);
  print_node (m);
  mapdb->free (m);

  // 
  // server [8M+4K] -> client [8K]
  // 
  assert (client->v_lookup (0x8000, &phys, &size, &page_attribs) == false);

  fpage_map (server, 
	     Test_fpage (true, true, Config::PAGE_SHIFT, 0x801000),
	     client,
	     L4_fpage (L4_fpage::Whole_space, 0),
	     0x8000, false);

  assert (client->v_lookup (0x8000, &phys, &size, &page_attribs) == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Space::Page_writable 
			   | Space::Page_user_accessible));

  cout << "XXX: The 4K submapping is attached to the Sigma0 parent." << endl;
  m = mapdb->lookup (sigma0_space->id(), 0x401000, 0x401000);
  print_node (m);
  mapdb->free (m);

  // 
  // Delete tasks
  // 
  delete server;
  delete client;
  delete sigma0_space;

  cerr << "OK" << endl;

  return 0;
}

static void print_node(Mapping* node, int depth = 0)
{
  assert (node);

  for (int i = depth; i != 0; i--)
    cout << ' ';

  cout << setbase(16)
       << "space=0x"  << space_number (node->space())
       << " vaddr=0x" << node->vaddr()
       << " size=0x" << node->size()
       << " type=0x" << node->type();

  unsigned after = 0;
  Mapping* next = node;
  while ((next = next->next_iter()))
    after++;

  cout << " after=" << after << endl;

  next = node;
  while ((next = next->next_child(node)))
    {
      if(next->parent() == node)
	print_node (next, depth + 1);
    }

  if (depth == 0)
    cout << endl;
}
