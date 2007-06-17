IMPLEMENTATION:

#include "per_cpu_data.h"

class Per_cpu_data_alloc : public Per_cpu_data
{
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include <cstdio>
#include <cstring>

#include "kmem_alloc.h"
#include "static_init.h"


PUBLIC static
void
Per_cpu_data_alloc::init()
{
  unsigned num_cpus = 2;
  _num_cpus = num_cpus;

  extern char _per_cpu_data_start[];
  extern char _per_cpu_data_end[];

  unsigned size = _per_cpu_data_end - _per_cpu_data_start;

  char *per_cpu = (char*)Kmem_alloc::allocator()
    ->unaligned_alloc(size * num_cpus);

  printf("Allocate %u bytes (%uKB) for CPU local storage\n"
         "  master copy @ %p\n",
	 num_cpus * size, (size * num_cpus + 513) / 1024,
	 _per_cpu_data_start);

  for (unsigned i = 0; i < num_cpus; ++i)
    {
      memcpy(per_cpu + size * i, _per_cpu_data_start, size);
      _offsets[i] = per_cpu + size * i - _per_cpu_data_start;
      printf("  CPU(%u) @ %p (offset=%lx)\n", i, per_cpu + size *i,
	  _offsets[i]);
    }

  memset(_per_cpu_data_start, 0x55, size);
}

STATIC_INITIALIZE_P(Per_cpu_data_alloc, CPU_LOCAL_BASE_INIT_PRIO);

