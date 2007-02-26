
/*****************************************************************************/
/**
 * \brief  Init L4Ka/Hazelnut
 */
/*****************************************************************************/ 

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <l4/sys/kernel.h>

#include "init.h"
#include "startup.h"

void
init_hazelnut(l4_kernel_info_t ** l4i)
{
  unsigned char * p;

  *l4i = 0;

  printf("  find kernel info page...\n");
  for (p = (unsigned char *) (kernel_low & 0xfffff000); 
       p < (unsigned char *) kernel_low + (kernel_high - kernel_low); 
       p += 0x1000)
    {
      l4_umword_t magic = L4_KERNEL_INFO_MAGIC;
      if (memcmp(p, &magic, 4) == 0)
	{
	  *l4i = (l4_kernel_info_t *) p;
	  printf("  found kernel info page at %p\n", p);
	  break;
	}
    }
  assert(*l4i);

  (*l4i)->reserved0.low = kernel_low;
  (*l4i)->reserved0.high = kernel_high;
}

