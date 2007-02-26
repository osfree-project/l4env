IMPLEMENTATION [ia32]:

#include "globalconfig.h"
#include "kmem.h"
#include "long_msg.h"
#include "std_macros.h"


IMPLEMENT inline NEEDS ["kmem.h"]
Address
Thread::remote_fault_addr (Address pfa)
{
  return pfa < Kmem::ipc_window(1) ? pfa - Kmem::ipc_window(0) + _vm_window0
                                   : pfa - Kmem::ipc_window(1) + _vm_window1;
}

/**
 * Define contents of IPC windows.
 *
 * The address range set for these ranges is mapped upon the next page fault
 * in handle_page_fault().
 *
 * @param win number of IPC window -- either 0 or 1.
 * @param address 4-MByte-aligned virtual address of range to be mapped
 *                into given window
 */              
PROTECTED inline NEEDS ["config.h", "kmem.h", "std_macros.h"]
void             
Thread::setup_ipc_window(unsigned win, Address address)
{                
  if (win == 0)
    {
      // Useless, because _vm_window0 contains the always the message buffer,
      // which we setup for every long ipc only once.
      // And because every thread switch flushes always the IPC window.
      // we need to update _vm_window0 every time.
      // This can be enabled, if we get an ipc window owner per space, 
      // like the fpu.
      // if (EXPECT_TRUE (_vm_window0 == address))
      //   return;
      _vm_window0 = address;
    }
  else
    {
      // Only makes sense if some strings are in the same 4mb region.
      if (EXPECT_FALSE (_vm_window1 == address))
	return;
      _vm_window1 = address;
    }

  // Pull in the mappings for the entire 8 MB window, by copying 2 PDE slots,
  // thereby replacing the old ones, based on the optimistic assumption that
  // the receiver's mappings are already set up appropriately. Note that this
  // does not prevent a pagefault on either of these mappings later on, e.g.
  // if the receiver's mapping is r/o here and needs to be r/w for Long-IPC.
  // Careful: for SMAS current_space() != space()
  current_space()->remote_update (Kmem::ipc_window (win),
                                  receiver()->space(), address, 2);
}

