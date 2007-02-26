/*
 * ARM Kernel-Info Page
 */

INTERFACE:

#include "types.h"

enum {
  L4_MEMORY_DESC_TYPE_MASK = 0x00ff,
  L4_MEMORY_DESC_SIZE_MASK = 0xffffff00,

  L4_MEMORY_DESC_AVAIL     = 0x00,
  L4_MEMORY_DESC_BOOT      = 0x01,
  L4_MEMORY_DESC_RESERVED  = 0x02,
};

struct Kernel_memory_desc
{
  void   *base; // base address
  Mword size_type; // in Bytes
};



EXTENSION class Kernel_info
{
public:
  Mword magic;
  Mword version;
  Mword offset_version_strings;
  Mword offset_memory_descs;

  char vstrs[256];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */
  Mword sigma0_sp, sigma0_pc;
  Mword sigma1_sp, sigma1_pc;
  Mword root_sp,   root_pc;

  size_t total_ram;
  volatile Cpu_time clock;
};


IMPLEMENTATION[arm]:
//-
