/*
 * Some generic functions for LCD handling.
 */

#include <l4/l4rm/l4rm.h>
#include <l4/sigma0/kip.h>
#include <l4/arm_drivers/lcd.h>
#include <stdlib.h>
#include <stdio.h>

enum { nr_drivers = 10 };
static struct arm_lcd_ops *ops[nr_drivers];
static int ops_alloced;

struct arm_lcd_ops *arm_lcd_probe(void)
{
  int i;

  for (i = 0; i < ops_alloced; i++)
    if (ops[i]->probe && ops[i]->probe())
      return ops[i];

  return NULL;
}

void arm_lcd_register_driver(struct arm_lcd_ops *_ops)
{
  if (ops_alloced < nr_drivers)
    ops[ops_alloced++] = _ops;
}

int arm_lcd_get_region(l4_addr_t addr, l4_size_t size)
{
  l4_threadid_t sigma0_id = L4_NIL_ID;

  sigma0_id.id.task    = 2;
  sigma0_id.id.lthread = 0;

  if (l4rm_area_setup_region(addr, size,
                             L4RM_DEFAULT_REGION_AREA, L4RM_REGION_PAGER,
			     0, sigma0_id))
    {
      printf("l4rm_area_setup_region failed for %08lx\n", addr);
      return 1;
    }
  return 0;
}
