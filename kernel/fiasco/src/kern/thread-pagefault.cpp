IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "logdefs.h"
#include "processor.h"
#include "regdefs.h"
#include "std_macros.h"
#include "thread.h"
#include "vmem_alloc.h"
#include "warn.h"


/** 
 * The global page fault handler switch.
 * Handles page-fault monitoring, classification of page faults based on
 * virtual-memory area they occured in, page-directory updates for kernel
 * faults, IPC-window updates, and invocation of paging function for
 * user-space page faults (handle_page_fault_pager).
 * @param pfa page-fault virtual address
 * @param error_code CPU error code
 * @return true if page fault could be resolved, false otherwise
 * @exception longjmp longjumps to recovery location if page-fault
 *                    handling fails (i.e., return value would be false),
 *                    but recovery location has been installed           
 */
IMPLEMENT inline NEEDS[<cstdio>,"regdefs.h", "kdb_ke.h","processor.h",
		       "config.h","std_macros.h","vmem_alloc.h","logdefs.h",
		       "warn.h",Thread::page_fault_log]
int Thread::handle_page_fault (Address pfa, Mword error_code, Mword pc,
    Return_frame *regs)
{
  if (Config::Log_kernel_page_faults && !PF::is_usermode_error(error_code))
    {
      Lock_guard<Cpu_lock> guard(&cpu_lock);
      printf("*KP[%lx,", pfa);
      print_page_fault_error(error_code);
      printf(",%lx]\n", pc);
    }
#if 0
  printf("Translation error ? %x\n"
	 "  current space has mapping : %08x\n"
	 "  Kernel space has mapping  : %08x\n",
	 PF::is_translation_error(error_code),
	 current_mem_space()->lookup((void*)pfa),
	 Space::kernel_space()->lookup((void*)pfa));
#endif

  if (Config::monitor_page_faults)
    {
      if (_last_pf_address == pfa && _last_pf_error_code == error_code)
        {
          if (!log_page_fault())
            printf("*P[%lx,%lx,%lx]", pfa, error_code & 0xffff, pc);
	  putchar('\n');

          kdb_ke("PF happened twice");
        }

      _last_pf_address = pfa;
      _last_pf_error_code = error_code;

      // (See also corresponding code in Thread::handle_ipc_page_fault()
      //                          and in Thread::handle_slow_trap.)
    }

  CNT_PAGE_FAULT;

  // TODO: put this into a debug_page_fault_handler
  if (EXPECT_FALSE (log_page_fault()))
    page_fault_log (pfa, error_code, pc);

  Ipc_err ipc_code(0);

  // Check for page fault in user memory area
  if (EXPECT_TRUE (!Kmem::is_kmem_page_fault(pfa, error_code)))
    {
      // Make sure that we do not handle page faults that do not
      // belong to this thread.
      assert (mem_space() == current_mem_space());

      if (EXPECT_FALSE (mem_space()->is_sigma0()))
        {
          // special case: sigma0 can map in anything from the kernel
	  if(handle_sigma0_page_fault(pfa))
            return 1;

          ipc_code.error (Ipc_err::Remapfailed);
          goto error;
        }

      // user mode page fault -- send pager request
      if (!(ipc_code 
	    = handle_page_fault_pager(_pager, pfa, error_code)).has_error())
        return 1;

      goto error;
    }

  // Check for page fault in small address space
  else if (EXPECT_FALSE (Kmem::is_smas_page_fault(pfa)))
    {
      if (handle_smas_page_fault (pfa, error_code, ipc_code))
        return 1;

      goto error;
    }
  
  // Check for page fault in kernel memory region caused by user mode
  else if (EXPECT_FALSE(PF::is_usermode_error(error_code)))
    return 0;             // disallow access after mem_user_max

  // Check for page fault in IO bit map or in delimiter byte behind IO bitmap
  // assume it is caused by an input/output instruction and fall through to
  // handle_slow_trap
  else if (EXPECT_FALSE(Kmem::is_io_bitmap_page_fault(pfa)))
    return 0;

  // We're in kernel code faulting on a kernel memory region

  // Check for page fault in IPC window. Mappings for this should never
  // be present in the global master page dir (case below), because the
  // IPC window mappings are always flushed on context switch.
  else if (EXPECT_TRUE (Kmem::is_ipc_page_fault(pfa, error_code)))
    {
      if (!(ipc_code = handle_ipc_page_fault(pfa, error_code)).has_error())
        return 1;

      goto error;
    }

  // A page is not present but a mapping exists in the global page dir.
  // Update our page directory by copying from the master pdir
  // This is the only path that should be executed with interrupts
  // disabled if the page faulter also had interrupts disabled.   
  // thread_page_fault() takes care of that.
  else if (PF::is_translation_error(error_code) &&
#ifdef CONFIG_ARM
      Mem_space::kernel_space()->lookup((void*)pfa) != ~0UL
#else
      Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != ~0UL
#endif
      )
    {
      current_mem_space()->kmem_update((void*)pfa);
      return 1;
    }

  // Check for page fault in kernel's TCB area
  else if (Kmem::is_tcb_page_fault(pfa, error_code))
    {
      // Test for special case -- see function documentation
      if (pagein_tcb_request(regs))
	 return 2;

      if (cpu_lock.test())
	{
	  //LOG_MSG_3VAL(current(), "Bad", pfa, error_code, pc);
	  kdb_ke("Forbidden page fault under CPU lock! FIX ME!");
	}

      //LOG_MSG_3VAL(current(), "TCBA", pfa, pc, 0);
      kdb_ke("Implicit TCB alloc");

      current_mem_space()->kmem_update((void*)pfa);
      return 1;
    }

  WARN("No page fault handler for 0x%lx, error 0x%lx, pc "L4_PTR_FMT"",
        pfa, error_code, pc);

  // An error occurred.  Our last chance to recover is an exception
  // handler a kernel function may have set.
 error:

  if (_recover_jmpbuf)
    longjmp (*_recover_jmpbuf, ipc_code.raw());

  return 0;
}
