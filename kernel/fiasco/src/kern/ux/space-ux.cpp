/*
 * Fiasco-UX
 * Architecture specific pagetable code
 */

IMPLEMENTATION[ux]:

#include <unistd.h>
#include <sys/mman.h>
#include "boot_info.h"
#include "trampoline.h"

IMPLEMENT inline NEEDS [<unistd.h>, <sys/mman.h>, "boot_info.h", "trampoline.h"]
void Space::page_map (Address phys, Address virt, Address size,
                      unsigned page_attribs)
{
  Trampoline::mmap
    (pid(), virt, size,
     PROT_READ | (page_attribs & Page_writable ? PROT_WRITE : 0),
     MAP_SHARED | MAP_FIXED, Boot_info::fd(), phys);
}

IMPLEMENT inline NEEDS ["trampoline.h"]
void
Space::page_unmap (Address virt, Address size)
{
  Trampoline::munmap (pid(), virt, size);
}

IMPLEMENT inline NEEDS [<unistd.h>, <sys/mman.h>, "boot_info.h", "trampoline.h"]
void
Space::page_protect (Address virt, Address size,
                     unsigned page_attribs)
{
  Trampoline::mprotect (pid(), virt, size,
                        PROT_READ | (page_attribs & Page_writable ? PROT_WRITE : 0));
}

IMPLEMENT inline
void   
Space::kmem_update (void *)
{}

IMPLEMENT inline
void
Space::remote_update (const Address, const Space_context *, const Address, size_t)
{}

IMPLEMENT inline
void
Space::update_small (Address, bool)
{}
