#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/sigma0/sigma0.h>
#include <l4/arm_drivers/common.h>

#include <stdio.h>

int arm_driver_reserve_region(l4_addr_t addr, l4_size_t size)
{
  if (l4rm_area_setup_region(addr, size,
                             L4RM_DEFAULT_REGION_AREA, L4RM_REGION_PAGER,
                             0, l4sigma0_id()))
    {
      printf("l4rm_area_setup_region failed for %08lx\n", addr);
      return 1;
    }

  return 0;
}

/* map some io memory to some arbitrary virtual address */
l4_addr_t arm_driver_map_io_region(l4_addr_t phys, l4_size_t size)
{
  int ret;
  l4_addr_t virt;

  ret = l4rm_area_setup(size, L4RM_DEFAULT_REGION_AREA,
                        L4RM_REGION_PAGER, L4RM_LOG2_ALIGNED,
                        L4_INVALID_ID, &virt);
  if (ret)
    {
      printf("l4rm_area_setup failed with %d(%s)\n",
             ret, l4env_strerror(ret));
      return -1;
    }

  ret = l4sigma0_map_iomem(l4sigma0_id(), phys, virt, size, 0);
  if (ret)
    {
      printf("l4sigma0_map_iomem failed with %d(%s)\n",
             ret, l4sigma0_map_errstr(ret));
    }

  return virt;
}
