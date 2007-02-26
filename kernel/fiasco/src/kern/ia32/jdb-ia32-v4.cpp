IMPLEMENTATION[ia32-v4]:


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
 
  // It would be too slow if we ask the kip for each address if it is 
  // reserved, so we count page-wise and ask only once per page.

  Address a, a_top;
  Mword page, page_top;
  Mword off, off_top;

  // the first address/page/offset to search in
  a        = (*address) ? *address : Kmem::mem_phys;
  page     = a >> Config::PAGE_SHIFT;
  off      = a & ~Config::PAGE_MASK;

  // the last address/page/offset to search in
  a_top    = Kmem::mem_phys + Kmem::info()->main_memory_high() - 4;
  page_top = a_top >> Config::PAGE_SHIFT;
  off_top  = a_top & ~Config::PAGE_MASK;

  if (*address >= a_top) return false;

  // search in 1st page
  if (Kmem::info()->mem_is_reserved_or_shared 
      (Kmem::virt_to_phys ((void*)(page << Config::PAGE_SHIFT)))) 
    { page++; off=0; }
  else 
    for (; off < Config::PAGE_SIZE; off++)
      if (*(unsigned*)((page << Config::PAGE_SHIFT) | off) == string)
	{ 
	  *address = (page << Config::PAGE_SHIFT) | off; 
	  return true; 
	}
  page++;

  // search in the pages between
  for (;page < page_top; page++)
    if ( ! Kmem::info()->mem_is_reserved_or_shared 
	 (Kmem::virt_to_phys ((void*)(page << Config::PAGE_SHIFT))))
      for (off=0; off < Config::PAGE_SIZE ; off++)
	if (*(unsigned*)((page << Config::PAGE_SHIFT) | off) == string)
	  { 
	    *address = (page << Config::PAGE_SHIFT) | off; 
	    return true; 
	  }

  // search in last page
  assert (page == page_top);
  if ( ! Kmem::info()->mem_is_reserved_or_shared 
       (Kmem::virt_to_phys ((void*)(page << Config::PAGE_SHIFT))))
    for (off=0; off < off_top; off++)
      if (*(unsigned*)((page << Config::PAGE_SHIFT) | off) == string)
	{ 
	  *address = (page << Config::PAGE_SHIFT) | off; 
	  return true; 
	}
  
  return false;
}

