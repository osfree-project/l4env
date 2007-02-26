IMPLEMENTATION[ia32-v2x0]:

/// Dummy.  For now you can't access an UTCB through gs[0] in v2/x0 ABI
IMPLEMENT FIASCO_INIT void Kmem::setup_gs0_page() {}

/// v2/x0 ABI specific KIP initialization
IMPLEMENT FIASCO_INIT
void Kmem::setup_kip_abi (Kernel_info *kinfo)
{
  extern char __crt_dummy__, _end; // defined by linker and in crt0.S

  kinfo->main_memory.low = 0;
  kinfo->main_memory.high = mem_max;
  kinfo->reserved0.low = virt_to_phys(&__crt_dummy__) & Config::PAGE_MASK;
  kinfo->reserved0.high = (virt_to_phys(&_end) + Config::PAGE_SIZE -1) 
    & Config::PAGE_MASK;
  kinfo->semi_reserved.low = 1024 * Boot_info::mbi_virt()->mem_lower;
  kinfo->semi_reserved.high = 1024 * 1024;

  kinfo->clock = 0;

  // now define space for the kernel memory allocator
  kinfo->reserved1.low  = Boot_info::kmem_start((1LL<<32) - mem_phys);
  kinfo->reserved1.high = mem_max;
}

