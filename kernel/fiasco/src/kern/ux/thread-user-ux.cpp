
/*
 * Fiasco-UX
 * Architecture specific user memory access code.
 */

INTERFACE:

EXTENSION class Thread
{
private:
  template < typename T >
  T * user_to_kernel (T const *addr, bool write);
};

IMPLEMENTATION[user-ux]:

#include "config.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "processor.h"
#include "regdefs.h"
#include "undef_oskit.h"

/**
 * Transform user pointer to kernel pointer by translating the user
 * virtual address to a physical address and then to a kernel virtual address.
 * Raise a pagefault if translation fails.
 * @param addr user pointer to transform
 * @param write true if write access will be performed through the pointer
 * @returns corresponding kernel pointer
 */
IMPLEMENT 
template < typename T > T *
Thread::user_to_kernel (T const *addr, bool write) {

  Address  phys;
  size_t   size;
  unsigned attr, error = 0;

  while (1) {

    // See if there is a mapping for this address
    if (space()->v_lookup ((Address) addr, &phys, &size, &attr)) {

      // A frame was found, add the offset
      if (size == Config::SUPERPAGE_SIZE)
        phys |= ((Address) addr & ~Config::SUPERPAGE_MASK);
      else
        phys |= ((Address) addr & ~Config::PAGE_MASK);

      // See if we want to write and are not allowed to
      // Generic check because INTEL_PTE_WRITE == INTEL_PDE_WRITE
      if (!write || (attr & INTEL_PTE_WRITE))
        return (T *) Kmem::phys_to_virt (phys);

      error |= PF_ERR_PRESENT;
    }

    if (write)
      error |= PF_ERR_WRITE;

    // If we tried to access user memory of a thread which is not current()
    // our Long-IPC partner must do a page-in. This is analogue to IA32
    // page-faulting in the IPC window. Set PF_ERR_REMTADDR hint.
    // Otherwise we faulted on our own user memory. Set PF_ERR_USERADDR hint.
    error |= (this == current() ? PF_ERR_USERADDR : PF_ERR_REMTADDR);

    // No mapping or insufficient access rights, raise pagefault.
    // PF handler opens interrupts, we restore the current state afterwards.
    {
      Cpu_lock::Status was_locked = cpu_lock.test();

      thread_page_fault (0, (Address) addr, error,
                         0, 0, 0, 0, 0, 0,	// EDX, ECX, EAX, EBP, EIP, CS
                         Proc::processor_state());

      cpu_lock.set (was_locked);
    }
  }
}   

/**
 * Return integral type T from virtual user address addr
 * @param addr virtual user address
 * @returns integral type T
 * @pre size of T must be a power of 2
 */
IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h", "undef_oskit.h"]
template< typename T > T
Thread::peek_user (T const *addr)
{
  T value;

  // Check if we have to read across page boundaries
  if (((Address) addr & Config::PAGE_MASK) + sizeof (T) <= Config::PAGE_SIZE) {

    // While we do page translation and copying, we must be protected
    // against someone removing the page mapping from underneath us.
    Lock_guard<Cpu_lock> guard (&cpu_lock);
    value = *user_to_kernel (addr, false);

  } else
    copy_from_user<T>(&value, addr, 1);
  
  return value;
}

/**
 * Store integral type T at virtual user address addr
 * @param addr virtual user address
 * @param value integral T to store
 * @pre size of T must be a power of 2
 */
IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h", "undef_oskit.h"]
template< typename T > void
Thread::poke_user (T *addr, T value)
{
  // Check if we have to write across page boundaries
  if (((Address) addr & Config::PAGE_MASK) + sizeof (T) <= Config::PAGE_SIZE) {

    // While we do page translation and copying, we must be protected
    // against someone removing the page mapping from underneath us.
    Lock_guard<Cpu_lock> guard (&cpu_lock);
    *user_to_kernel (addr, true) = value;

  } else
    copy_to_user<T>(addr, &value, 1);
}

/**
 * Copy n elements of type T from user to kernel memory.
 * When crossing page boundaries, re-translate addresses.
 * @param kdst virtual kernel destination address
 * @param usrc virtual user source address
 * @param n number of elements of type T to copy
 */
IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h", "undef_oskit.h"]
template< typename T > void
Thread::copy_from_user (void *kdst, void const *usrc, size_t n)
{
  // While we do page translation and copying, we must be protected
  // against someone removing the page mapping from underneath us.
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  char *ptr = (char *) usrc;
  char *dst = (char *) kdst;
  char *src = user_to_kernel (ptr, false);

  n *= sizeof (T);

  while (n--) {

    *dst++ = *src++; ptr++;

    if (((Address) ptr & (Config::PAGE_SIZE - 1)) == 0)
      src = user_to_kernel (ptr, false);
  }
}

/**
 * Copy n elements of type T from kernel to user memory.
 * When crossing page boundaries, re-translate addresses.
 * @param udst virtual user destination address
 * @param ksrc virtual kernel source address
 * @param n number of elements of type T to copy
 */
IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h", "undef_oskit.h"]
template< typename T > void
Thread::copy_to_user (void *udst, void const *ksrc, size_t n)
{
  // While we do page translation and copying, we must be protected
  // against someone removing the page mapping from underneath us.
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  char *ptr = (char *) udst;
  char *src = (char *) ksrc;
  char *dst = user_to_kernel (ptr, true);

  n *= sizeof (T);

  while (n--) {

    *dst++ = *src++; ptr++;

    if (((Address) ptr & (Config::PAGE_SIZE - 1)) == 0)
      dst = user_to_kernel (ptr, true);
  }
}
