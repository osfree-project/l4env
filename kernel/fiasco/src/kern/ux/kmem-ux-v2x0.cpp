IMPLEMENTATION[ux-v2x0]:

/// Dummy.  For now you can't access an UTCB through gs[0] in v2/x0 ABI
IMPLEMENT FIASCO_INIT void Kmem::setup_gs0_page() {}

/// v2/x0 ABI specific KIP initialization
IMPLEMENT FIASCO_INIT
void Kmem::setup_kip_abi (Kernel_info *kinfo)
{
  kinfo->main_memory.low	= 0;
  kinfo->main_memory.high	= mem_max;
  kinfo->reserved0.low		= Emulation::kernel_start_frame;
  kinfo->reserved0.high		= Emulation::kernel_end_frame;
  kinfo->reserved1.low		= Boot_info::kmem_start(mem_max);
  kinfo->reserved1.high		= mem_max;
  kinfo->semi_reserved.low	= 1024 * Boot_info::mbi_virt()->mem_lower;
  kinfo->semi_reserved.high	= 1024 * 1024;

  kinfo->clock			= 0;

  kinfo->dedicated[0].low       = kinfo->root_memory.low;
  kinfo->dedicated[0].high      = kinfo->root_memory.high;

  for (int i = 1; i < 4; i++)
    kinfo->dedicated[i].low = kinfo->dedicated[i].high = 0;         
}

