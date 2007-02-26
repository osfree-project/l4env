/*
 * Some convenience functions for memory descriptors.
 */

#include <l4/sigma0/kip.h>
#include <l4/util/memdesc.h>

l4_addr_t
l4util_memdesc_vm_high(void)
{
  l4_kernel_info_t * kinfo = l4sigma0_kip_map(L4_INVALID_ID);
  l4_kernel_info_mem_desc_t *md = l4_kernel_info_get_mem_descs(kinfo);
  int nr = l4_kernel_info_get_num_mem_descs(kinfo);
  int i;

  for (i = 0; i < nr; i++, md++)
    if (l4_kernel_info_get_mem_desc_is_virtual(md))
      return l4_kernel_info_get_mem_desc_end(md);

  return 0;
}
