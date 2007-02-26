IMPLEMENTATION[ia32,amd64,ux]:

/** Allocate size pages of kernel memory and map it sequentially
    to the given address space, starting at  virt_addr.
    @param space Target address space.
    @param virt_addr Virtual start address.
    @param size Size in pages.
    @param page_attributes Page table attributes per allocated page.
    @see local_free()
 */
PUBLIC static
bool
Vmem_alloc::local_alloc(Mem_space *space,
		        Address virt_addr, 
			int size,
		//	char fill,  
			unsigned int page_attributes)
{
  assert((virt_addr & (Config::PAGE_SIZE -1)) == 0);
  (void)page_attributes;
  
  Address va = virt_addr;
  int i;

  for (i=0; i<size; i++) 
    {
      void *page = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);

      if(!page)
	break;
    
      check(space->v_insert(Kmem::virt_to_phys(page), va, Config::PAGE_SIZE, 
			    page_attributes) == Mem_space::Insert_ok);

      memset(page, 0, Config::PAGE_SIZE);

      va += Config::PAGE_SIZE;
    }
  
  if(i == size) 
    return true;

  /* kdb_ke ("Vmem_alloc::local_alloc() failed");  */
  /* cleanup */
  local_free(space, virt_addr, i);

  return false;
}


/** Free the sequentially mapped memory in the given address space,
    starting at  virt_addr.
    The page table in the address space is not deleted. 
    @param space Target address space.
    @param virt_addr Virtual start address.
    @param size Size in pages.
    @see local_alloc()
 */
PUBLIC static
void 
Vmem_alloc::local_free(Mem_space *space, Address virt_addr, int size)
{
  assert((virt_addr & (Config::PAGE_SIZE -1)) == 0);

  Address va = virt_addr;
  
  for (int i=0; i<size; i++)
    {
      Address phys; 
      if (!space->v_lookup (va, &phys))
        kdb_ke ("not mapped");

      space->v_delete (va, Config::PAGE_SIZE); 
      Mapped_allocator::allocator()
	->free(Config::PAGE_SHIFT, Kmem::phys_to_virt (phys));
      va += Config::PAGE_SIZE;
    }
}
