IMPLEMENTATION:

#include <cstring>
#include "mem_layout.h"
#include "vmem_alloc.h"
#include "panic.h"

IMPLEMENT static 
void
Sys_call_page::init()
{
  Mword *sys_calls = (Mword*)Mem_layout::Syscalls;
  if (!Vmem_alloc::page_alloc(sys_calls, 
	Vmem_alloc::NO_ZERO_FILL, Vmem_alloc::User))
    panic("FIASCO: can't allocate system-call page.\n");

  for (unsigned i = 0; i < Config::PAGE_SIZE; i += sizeof(Mword))
    *(sys_calls++) = 0xef000000; // swi
  

}
