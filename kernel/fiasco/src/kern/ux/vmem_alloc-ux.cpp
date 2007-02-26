
IMPLEMENTATION[ux]:

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "boot_info.h"
#include "panic.h"

IMPLEMENT inline NEEDS [<errno.h>, <string.h>, <unistd.h>, <sys/mman.h>,
                        "boot_info.h", "config.h", "panic.h"]
void
Vmem_alloc::page_map (void *address, int order, Zero_fill zf, Address phys)
{
  if (mmap (address, (1 << order) * Config::PAGE_SIZE,
            PROT_READ | (zf == ZERO_FILL ? PROT_WRITE : 0),
            MAP_SHARED | MAP_FIXED,
            Boot_info::fd(), phys) == MAP_FAILED)
    panic ("mmap error: %s", strerror (errno));
}

IMPLEMENT inline NEEDS [<errno.h>, <string.h>, <unistd.h>, <sys/mman.h>,
                        "boot_info.h", "config.h", "panic.h"]
void
Vmem_alloc::page_unmap (void *address, int order)
{
  if (munmap (address, (1 << order) * Config::PAGE_SIZE) != 0)
    panic ("munmap error: %s", strerror (errno));
}
