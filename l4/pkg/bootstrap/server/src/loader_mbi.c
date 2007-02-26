/*
 * Used when our boot loader does not supply a mbi structure.
 */

#include <stdio.h>

#include "loader_mbi.h"

l4util_mb_info_t loader_mbi;

l4util_mb_info_t *init_loader_mbi(void *realmode_pointer, unsigned int mem)
{
  loader_mbi.flags      = L4UTIL_MB_MEMORY;
  loader_mbi.mem_lower  = 0x9f * 4;

  if (!mem)
    {
      loader_mbi.mem_upper  = *(unsigned long *)(realmode_pointer + 0x1e0);
      printf("Detected memory size: %dKB\n", loader_mbi.mem_upper);
    }
  else
    {
      loader_mbi.mem_upper  = mem;
      printf("Memory size: %dKB\n", loader_mbi.mem_upper);
    }

  return &loader_mbi;
}
