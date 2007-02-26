/**
 * \file
 */

#include "init_kip.h"
#include "startup.h"

/* XXX not possible to include kip.h from L4Ka::Pistachio here */

#if defined(ARCH_x86)
#define V4KIP_SIGMA0_SP         0x20
#define V4KIP_SIGMA0_IP         0x24
#define V4KIP_SIGMA0_LOW        0x28
#define V4KIP_SIGMA0_HIGH       0x2C
#define V4KIP_ROOT_SP           0x40
#define V4KIP_ROOT_IP           0x44
#define V4KIP_ROOT_LOW          0x48
#define V4KIP_ROOT_HIGH         0x4C
#define V4KIP_MEM_INFO		0x54
#define V4KIP_BOOT_INFO		0xB8
#elif defined(ARCH_amd64)
#define V4KIP_SIGMA0_SP         0x40
#define V4KIP_SIGMA0_IP         0x48
#define V4KIP_SIGMA0_LOW        0x50
#define V4KIP_SIGMA0_HIGH       0x58
#define V4KIP_ROOT_SP           0x80
#define V4KIP_ROOT_IP           0x88
#define V4KIP_ROOT_LOW          0x90
#define V4KIP_ROOT_HIGH         0x98
#define V4KIP_MEM_INFO		0xA8
#define V4KIP_BOOT_INFO		0x170
#else
#error unknown architecture
#endif

#define V4KIP_WORD(kip,offset)	((l4_umword_t*)(((char*)(kip))+(offset)))

/**
 * @brief Initialize KIP prototype for Fiasco/v4.
 */
void
init_kip_v4 (void *l4i, boot_info_t *bi, l4util_mb_info_t *mbi)
{
  l4_umword_t *p_meminfo = V4KIP_WORD(l4i,V4KIP_MEM_INFO);
  l4_umword_t *p         = V4KIP_WORD(l4i,(*p_meminfo >> 16));
  l4_addr_t   mem_end    = (1<<20) + bi->mem_upper * 1024UL;
  int         num_info   = 0;

  // 1: all memory is accessible for users
  *p++ = 0 | 4;
  *p++ = ~0UL;
  num_info++;

  // 2: conventional low memory
  *p++ = 0 | 1;
  *p++ = bi->mem_lower*1024UL - 1;
  num_info++;

  // 3: conventional high memory
  *p++ = (1<<20) | 1;
  *p++ = mem_end;
  num_info++;

  // 4: (additional) 20MB kernel memory
  if (mem_end > (240 << 20))
      mem_end = 240 << 20;
  *p++ = (mem_end - (20 << 20)) | 2;
  *p++ = mem_end;
  num_info++;

  // 5: VGA
  *p++ = 0xA0000 | 4;
  *p++ = 0xBFFFF;
  num_info++;

  // 6: BIOS
  *p++ = 0xC0000 | 4;
  *p++ = 0xEFFFF;
  num_info++;

  if (bi->sigma0_low)
    {
      // 7: reserve sigma0 memory
      *p++ = bi->sigma0_low | 3;
      *p++ = bi->sigma0_high;
      num_info++;
      *V4KIP_WORD(l4i,V4KIP_SIGMA0_LOW)  = bi->sigma0_low;
      *V4KIP_WORD(l4i,V4KIP_SIGMA0_HIGH) = bi->sigma0_high;
      *V4KIP_WORD(l4i,V4KIP_SIGMA0_IP)   = bi->sigma0_start;
    }

  if (bi->roottask_low)
    {
      // 8: reserve roottask memory
      *p++ = bi->roottask_low | 3;
      *p++ = bi->roottask_high;
      num_info++;
      *V4KIP_WORD(l4i,V4KIP_ROOT_LOW)  = bi->roottask_low;
      *V4KIP_WORD(l4i,V4KIP_ROOT_HIGH) = bi->roottask_high;
      *V4KIP_WORD(l4i,V4KIP_ROOT_IP)   = bi->roottask_start;
      *V4KIP_WORD(l4i,V4KIP_ROOT_SP)   = bi->roottask_stack;
    }

  *V4KIP_WORD(l4i,V4KIP_BOOT_INFO) = (unsigned long)mbi;

  // set the mem_info variable: count of mem descriptors
  *p_meminfo = (*p_meminfo & 0xffff0000) | num_info;
}
