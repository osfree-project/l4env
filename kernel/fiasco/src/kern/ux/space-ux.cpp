/*
 * Fiasco-UX
 * Architecture specific pagetable code
 */

INTERFACE[ux]:
#include <sys/types.h>		// for pid_t
#include "kmem.h"
#include "mem_layout.h"
#include "paging.h"

EXTENSION class Space
{
public:
  pid_t pid() const;
};



IMPLEMENTATION[ux]:

#include <unistd.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include "boot_info.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "mem_layout.h"
#include "trampoline.h"
#include <cstring>
#include "config.h"
#include "emulation.h"
#include "logdefs.h"
#include "space_index.h"

IMPLEMENT inline NEEDS ["kmem.h", "emulation.h"]
Space *
Space::current()
{
  return reinterpret_cast<Space *>
         (Kmem::phys_to_virt (Emulation::pdir_addr()));
}

IMPLEMENT inline NEEDS ["kmem.h", "emulation.h"]
void
Space::make_current()
{
  Emulation::set_pdir_addr (Kmem::virt_to_phys (this));
}

IMPLEMENT inline
pid_t
Space::pid() const			// returns host pid number
{
  return _dir.entry(Mem_layout::Pid_index).raw() >> 8;
}

IMPLEMENT inline NEEDS["logdefs.h"]
void
Space::switchin_context()
{
  if (Space::current() == this)
    return;

  switchin_lipc_kip_pointer();
  CNT_ADDR_SPACE_SWITCH;
  make_current();
}
/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  * 
  * @param new_number Task number of the new address space
  */
PROTECTED
Space::Space(unsigned number)
{
  _dir.clear();
  // Scribble task/chief numbers into an unused part of the page directory.
  // Assure that the page directory entry is not valid!
  *(_dir.lookup(Mem_layout::Space_index)) = number << 8;
  *(_dir.lookup(Mem_layout::Chief_index)) = Space_index(number).chief() << 8;

  // register in task table
  Space_index::add (this, number);
}

IMPLEMENT inline NEEDS [<asm/unistd.h>, <sys/mman.h>, "boot_info.h",
                        "cpu_lock.h", "lock_guard.h", "mem_layout.h",
                        "trampoline.h"]
void
Space::page_map (Address phys, Address virt, Address size, unsigned attr)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  Mword *trampoline = (Mword *) Mem_layout::kernel_trampoline_page;

  *(trampoline + 1) = virt;
  *(trampoline + 2) = size;
  *(trampoline + 3) = PROT_READ | (attr & Page_writable ? PROT_WRITE : 0);
  *(trampoline + 4) = MAP_SHARED | MAP_FIXED;
  *(trampoline + 5) = Boot_info::fd();

  if (phys >= Boot_info::fb_virt() &&
      phys + size <= Boot_info::fb_virt() +
                     Boot_info::fb_size() +
                     Boot_info::input_size())
    *(trampoline + 6) = Boot_info::fb_phys() + (phys - Boot_info::fb_virt());
  else
    *(trampoline + 6) = phys;

  Trampoline::syscall (pid(), __NR_mmap, Mem_layout::Trampoline_page + 4);
}

IMPLEMENT inline NEEDS [<asm/unistd.h>, "cpu_lock.h", "lock_guard.h",
                        "trampoline.h"]
void
Space::page_unmap (Address virt, Address size)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  Trampoline::syscall (pid(), __NR_munmap, virt, size);
}

IMPLEMENT inline NEEDS [<asm/unistd.h>, "cpu_lock.h", "lock_guard.h",
                        "trampoline.h"]
void
Space::page_protect (Address virt, Address size, unsigned attr)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  Trampoline::syscall (pid(), __NR_mprotect, virt, size,
                       PROT_READ | (attr & Page_writable ? PROT_WRITE : 0));
}

IMPLEMENT inline
void   
Space::kmem_update (void *)
{}

IMPLEMENT inline
void
Space::remote_update (const Address, const Space *, const Address, size_t)
{}

IMPLEMENT inline
void
Space::update_small (Address, bool)
{}
