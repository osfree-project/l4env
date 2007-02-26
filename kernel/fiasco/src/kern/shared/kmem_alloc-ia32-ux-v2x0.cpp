IMPLEMENTATION[ia32-ux-v2x0]:

static lmm_region_t lmm_region_all;


IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  // The next line must be replaced SOON
  void *kmem_base = Kmem::phys_to_virt (Kmem::info()->reserved1.low);

  vm_size_t kmem_size = 
    reinterpret_cast<Address>(Kmem::phys_to_virt(Kmem::himem())) 
    - reinterpret_cast<Address>(kmem_base);

  lmm_init((lmm_t*)lmm);
  lmm_add_region((lmm_t*)lmm, &lmm_region_all, (void*)0, (vm_size_t)-1, 0, 0);
  lmm_add_free((lmm_t*)lmm, kmem_base, kmem_size);

  //  printf("Kmem_alloc() finished!\n"
  //	 "  avail mem = %d bytes\n", lmm_avail(&lmm,0));
}


PUBLIC
void Kmem_alloc::debug_dump()
{
  Helping_lock_guard guard(&lmm_lock);

  lmm_dump((lmm_t*)lmm);
  vm_size_t free = lmm_avail((lmm_t*)lmm, 0);
  vm_size_t orig_free = Kmem::info()->reserved1.high 
                        - Kmem::info()->reserved1.low;
  printf("Used 0x%x/0x%x bytes (%d%%, %d/%dkB) of kmem\n", 
	 orig_free - free, 
	 orig_free,
	 (Unsigned32)(100LL*(orig_free-free)/orig_free),
	 (orig_free - free + 1023)/1024,
	 (orig_free        + 1023)/1024);
}
