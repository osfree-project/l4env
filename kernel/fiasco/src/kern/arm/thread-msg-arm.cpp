IMPLEMENTATION [arm]:

#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "long_msg.h"
#include "mem_unit.h"
#include "std_macros.h"


PROTECTED inline NEEDS ["config.h", "mem_unit.h", "kmem.h", "std_macros.h"]
void             
Thread::setup_ipc_window(unsigned win, Address address)
{                
  if (win == 0) {

    // Useless, because _vm_window0 contains the always the message buffer,
    // which we setup for every long ipc only once.
    // And because every thread switch flushes always the IPC window.
    // we need to update _vm_window0 every time.
    // This can be enabled, if we get an ipc window owner per space, 
    // like the fpu.
    // if (EXPECT_TRUE (_vm_window0 == address))
    //   return;
    _vm_window0 = address;

  } else {
    // Only makes sense if some strings are in the same 4mb region.
    if (EXPECT_FALSE (_vm_window1 == address))
      return;

    _vm_window1 = address;
  }

  // ATTENTION: Pulling in only the first 4MB of the IPC window to avoid
  // attacks where a receive buffer is just 4MB below the kernel address space
  // and global pages would be moved into the IPC window.
  
  // Pull in the mappings for the entire 8 MB window, by copying 2 PDE slots,
  // thereby replacing the old ones, based on the optimistic assumption that
  // the receiver's mappings are already set up appropriately. Note that this
  // does not prevent a pagefault on either of these mappings later on, e.g.
  // if the receiver's mapping is r/o here and needs to be r/w for Long-IPC.
  // Careful: for SMAS current_mem_space() != space()
  Page_table::current()->copy_in((void*)Kmem::ipc_window(win),
                           receiver()->mem_space()->dir(), 
                           (void*)address, 
                           Config::SUPERPAGE_SIZE * 2, 
			   current_mem_space()->c_asid());
}

IMPLEMENT inline
Address Thread::remote_fault_addr( Address pfa )
{
  return pfa < Kmem::ipc_window(1) ? pfa - Kmem::ipc_window(0) + _vm_window0
                                   : pfa - Kmem::ipc_window(1) + _vm_window1;
}

IMPLEMENT inline NEEDS["config.h", "mem_unit.h"]
Mword Thread::update_ipc_window (Address pfa, Address remote_pfa, Mword error)
{
  // XXX: We don't care about the page-fault error code here, but
  // we should: If we want to write to a read-only page, we would
  // fail here.  Space::update() probably should take an
  // error-code argument, as should
  // Thread::ipc_pagein_request().
  
  (void)error;

  Pte pte = receiver()->mem_space()->dir()->walk((void*)remote_pfa,0,false,0);

  if (pte.valid()
      && pte.attr().permits(Mem_page_attr::Write | Mem_page_attr::User))
    {
      mem_space()->dir()->copy_in((void*)pfa, 
				  receiver()->mem_space()->dir(), 
				  (void*)remote_pfa, 
				  Config::SUPERPAGE_SIZE, 
				  mem_space()->c_asid());

      cpu_lock.clear();

      // It's OK if the PF occurs again: This can happen if we're
      // preempted after the call to update() above.  (This code
      // corresponds to code in Thread::handle_page_fault() that
      // checks for double page faults.)
      if (Config::monitor_page_faults)
        {
          _last_pf_address = (Address) -1;
        }

      return 1;		// Success
    }
  return 0;		// Failure
}


