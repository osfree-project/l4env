IMPLEMENTATION[ia32-ux-v4]:

static lmm_region_t lmm_region_all;


IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  Mword kmem_size = Kmem::info()->main_memory_high()
    * Config::kernel_mem_per_cent / 100;
  if (kmem_size > Config::kernel_mem_max)
    kmem_size = Config::kernel_mem_max;

  void *kmem_base = reinterpret_cast<void*>
    ((Mword)(Kmem::phys_to_virt(Kmem::himem() - kmem_size)) 
     & Config::PAGE_MASK);
  
  Kmem::info()->mem_desc_add 
    (Kmem::virt_to_phys(kmem_base), Kmem::info()->main_memory_high(), 
     2, 0, 0);

  lmm_init((lmm_t*)lmm);
  lmm_add_region((lmm_t*)lmm, &lmm_region_all, (void*)0, (vm_size_t)-1, 0, 0);
  lmm_add_free((lmm_t*)lmm, kmem_base, kmem_size);

//   printf("Kmem_alloc() finished!\n"
//    "  avail mem = %d bytes\n", lmm_avail(&lmm,0));
}


PUBLIC
void Kmem_alloc::debug_dump()
{
  Helping_lock_guard guard(&lmm_lock);

  lmm_dump((lmm_t*)lmm);
  vm_size_t free = lmm_avail((lmm_t*)lmm, 0);

  // Hack: assume that the last descriptor is the Kmem region
  vm_size_t orig_free = 
    Kmem::info()->mem_desc_high (Kmem::info()->mem_desc_count() - 1) 
    - Kmem::info()->mem_desc_low (Kmem::info()->mem_desc_count() - 1);

  printf("Used 0x%x/0x%x bytes (%d%%, %d/%dkB) of kmem\n", 
	 orig_free - free, 
	 orig_free,
	 (Unsigned32)(100LL*(orig_free-free)/orig_free),
	 (orig_free - free + 1023)/1024,
	 (orig_free        + 1023)/1024);
}
