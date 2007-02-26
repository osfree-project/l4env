
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/sys/kip.h>

#include "rmgr.h"
#include "kinfo.h"

using L4::Kip::Mem_desc;

/*****************************************************************************
 *** Roottask internal API function
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return max. phys memory address
 *
 * \return Max. memory address
 */
/*****************************************************************************/ 

enum lowhigh {
  low, high
};

static
l4_addr_t
root_kinfo_get_conventional_mem(enum lowhigh t)
{
  l4_addr_t val = t == low ? ~0UL : 0UL;

  Mem_desc *md = Mem_desc::first(kip);
  Mem_desc *end = md + Mem_desc::count(kip);
  /* Find conventional memory in memory descriptors */
  for (; md != end; ++md)
    if (md->type() == Mem_desc::Conventional && !md->is_virtual())
      {
	switch (t)
	  {
	  case low:
	    if (md->start() < val)
	      val = md->start();
	    break;
	  case high:
	    if (md->end() > val)
	      val = md->end();
	    break;
	  }
      }

  return val;
}

l4_addr_t
root_kinfo_mem_high(void)
{
  return root_kinfo_get_conventional_mem(high) + 1;
}

l4_addr_t
root_kinfo_mem_low(void)
{
  return root_kinfo_get_conventional_mem(low);
}
