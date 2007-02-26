IMPLEMENTATION[ia32-v2x0]:

// Searches for a value in memory, starting at physical address if task==0.
// This means, that memory of all tasks is searched. Returns address of
// first apperance if found.
static bool
Jdb::search_dword(unsigned string, unsigned *address)
{
  // Search in physical memory
      
  // Funny things happen when reading from something like io-ports and 
  // display buffers so I skiped their memory locations (reserved windows) 
  // for searching.
  unsigned a, a_top;
  
  // search in <0> ... <start of Fiasco kernel>
  a     = (*address) ? *address : Kmem::mem_phys;
  a_top = (unsigned)Kmem::phys_to_virt(Kmem::info()->reserved0.low-4);
  if (*address < a_top)
    {
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }

  // search in <end of Fiasco kernel + 1> ... <0x000A0000>
  a     = (unsigned)Kmem::phys_to_virt(Kmem::info()->reserved0.high+1);
  a_top = (unsigned)Kmem::phys_to_virt(Kmem::info()->semi_reserved.low-4);
  if (*address < a_top)
    {
      if (a < *address)
	a = *address;
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }
  
  // search in <0x00100000> ... <end of main memory>
  a     = (unsigned)Kmem::phys_to_virt(Kmem::info()->semi_reserved.high+1);
  a_top = Kmem::mem_phys+Kmem::info()->main_memory_high()-4;
  if (*address < a_top)
    {
      if (*address > a)
	a = *address;
      for (; a < a_top; a++)
	if (*(unsigned*)a == string)
	  {
	    *address = a;
	    return true;
	  }
    }
  
  return false;
}

