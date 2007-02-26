IMPLEMENTATION:

#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;

#include "mapdb.h"

IMPLEMENTATION:

#include "config.h"

static unsigned
  s0 = Config::sigma0_taskno,
  other  = 10,
  client = 11;

static unsigned 
  grandfather = s0,
  father = 6, 
  son = 7, 
  daughter = 8, 
  aunt = 9;

static size_t page_sizes[] = { Config::SUPERPAGE_SIZE, Config::PAGE_SIZE };
static size_t page_sizes_max = 2;

static void print_node(Mapping* node, const Mapdb::Frame& frame)
{
  assert (node);

  size_t size = frame.size();

  for (Mapdb_iterator i (frame, node); node;)
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

void basic()
{
  Mapdb m (64, page_sizes, page_sizes_max);

  Mapping *node, *sub, *subsub;
  Mapdb::Frame frame;

  assert (! m.lookup(other, Config::PAGE_SIZE, Config::PAGE_SIZE, 
		     &node, &frame));

  cout << "Looking up 4M node at physaddr=0K" << endl;
  assert (m.lookup (s0, 0, 0, &node, &frame));
  print_node (node, frame);

  cout << "Inserting submapping" << endl;
  sub = m.insert (frame, node, other, 2*Config::PAGE_SIZE, Config::PAGE_SIZE, 
		  Config::PAGE_SIZE);
  print_node (node, frame);

  m.free (frame);

  //////////////////////////////////////////////////////////////////////

  cout << "Looking up 4M node at physaddr=8M" << endl;
  assert (m.lookup (s0, 2*Config::SUPERPAGE_SIZE, 2*Config::SUPERPAGE_SIZE,
		    &node, &frame));
  print_node (node, frame);

  // XXX broken mapdb: assert (node->size() == Config::SUPERPAGE_SIZE);

  cout << "Inserting submapping" << endl;
  sub = m.insert (frame, node, other, 
		  4*Config::SUPERPAGE_SIZE, 2*Config::SUPERPAGE_SIZE, 
		  Config::SUPERPAGE_SIZE);
  print_node (node, frame);

  assert (m.size(frame, sub) == Config::SUPERPAGE_SIZE);

  // Before we can insert new mappings, we must free the tree.
  m.free (frame);

  cout << "Get that mapping again" << endl;
  assert (m.lookup (other, 4*Config::SUPERPAGE_SIZE, 2*Config::SUPERPAGE_SIZE,
		    &sub, &frame));
  print_node (sub, frame);

  node = sub->parent();

  cout << "Inserting 4K submapping" << endl;
  subsub = m.insert (frame, sub, client, 15*Config::PAGE_SIZE, 2*Config::SUPERPAGE_SIZE, 
		     Config::PAGE_SIZE);
  print_node (node, frame);

  m.free (frame);
}

static void print_whole_tree(Mapping *node, const Mapdb::Frame& frame)
{
  while(node->parent())
    node = node->parent();
  print_node (node, frame);
}


void maphole()
{
  Mapdb m(1, page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map, *son_map, *daughter_map, *a_map;
  Mapdb::Frame frame;

  cout << "Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, 0, 0, &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, 0, 0, &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting son mapping" << endl;
  son_map = m.insert (frame, f_map, son, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, 0, 0, &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting daughter mapping" << endl;
  daughter_map = m.insert (frame, f_map, daughter, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "Looking up son at physaddr=0" << endl;
  assert (m.lookup(son, 0, 0, &son_map, &frame));
  f_map = son_map->parent();
  print_whole_tree (son_map, frame);

  cout << "Son has accident on return from disco" << endl;
  m.flush(frame, son_map, true, 0, 0, Config::PAGE_SIZE);
  m.free(frame);

  cout << "Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, 0, 0, &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting aunt mapping" << endl;
  a_map = m.insert (frame, gf_map, aunt, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Looking up daughter at physaddr=0" << endl;
  assert (m.lookup(daughter, 0, 0, &daughter_map, &frame));
  print_whole_tree (daughter_map, frame);
  f_map = daughter_map->parent();
  cout << "Father of daugther is " << (unsigned)(f_map->space()) << endl;

  assert(f_map->space() == father);

  m.free(frame);
}


void flushtest()
{
  Mapdb m(1, page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map, *son_map, *a_map;
  Mapdb::Frame frame;

  cout << "Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, 0, 0, &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, 0, 0, &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting son mapping" << endl;
  son_map = m.insert (frame, f_map, son, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (f_map, frame);
  m.free(frame);

  cout << "Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, 0, 0, &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting aunt mapping" << endl;
  a_map = m.insert (frame, gf_map, aunt, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup(father, 0, 0, &f_map, &frame));
  gf_map = f_map->parent();
  print_whole_tree (gf_map, frame);

  cout << "father is killed by his new love" << endl;
  m.flush(frame, f_map, true, 0, 0, Config::PAGE_SIZE);
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Try resurrecting the killed father again" << endl;
  assert (! m.lookup(father, 0, 0, &f_map, &frame));

  cout << "Resurrection is impossible, as it ought to be." << endl;
}

void multilevel ()
{
  size_t three_page_sizes[] = {0x40000000, 0x200000, 0x1000};
  Mapdb m (0x4, three_page_sizes, 3);

  Mapping *node /* , *sub, *subsub */;
  Mapdb::Frame frame;

  cout << "Looking up 0xd2000000" << endl;
  assert (m.lookup (s0, 0xd2000000, 0xd2000000, &node, &frame));

  print_node (node, frame);
}

#include "boot_info.h"
#include "cpu.h"
#include "config.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "static_init.h"
#include "vmem_alloc.h"

STATIC_INITIALIZER_P(init, STARTUP_INIT_PRIO);

static void init()
{
  Boot_info::init();
  Cpu::init();
  Config::init();
  Kmem::init();
  Kip_init::init();
  Kmem_alloc::init();
  Vmem_alloc::init();
}

int main()
{
  cout << "Basic test" << endl;
  basic();
  cout << "########################################" << endl;

  cout << "Hole test" << endl;
  maphole();
  cout << "########################################" << endl;

  cout << "Flush test" << endl;
  flushtest();
  cout << "########################################" << endl;

  cout << "Multilevel test" << endl;
  multilevel();
  cout << "########################################" << endl;

  cerr << "OK" << endl;
  return(0);
}
