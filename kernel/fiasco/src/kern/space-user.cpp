
/**
 * Functions to access user address space from inside the kernel.
 */

INTERFACE:

EXTENSION class Space
{
public:
  /**
   * Read integral type at any virtual address.
   * @param addr Virtual address in user or kernel address space.
   * @param user_space Location of virtual address (user or kernel space).
   * @return Integral value read from address.
   */
  template < typename T >
  T peek (T const *addr, bool user_space);

  /**
   * Read integral type at virtual address in user space.
   * @param addr Virtual address in user address space.
   * @return Integral value read from address.
   */
  template < typename T >
  T peek_user (T const *addr);

  /**
   * Set integral type at virtual address in user space to value.
   * @param addr Virtual address in user address space.
   * @param value New value to be set.
   */
  template < typename T >
  void poke_user (T *addr, T value);

  /**
   * Copy integral types from user space to kernel space.
   * @param kdst Virtual destination address in kernel space.
   * @param usrc Virtual source address in user space.
   * @param n Number of integral types to copy.
   */
  template < typename T >
  void copy_from_user (T *kdst, T const *usrc, size_t n);

  /**
   * Copy integral types from kernel space to user space.
   * @param udst Virtual destination address in user space.
   * @param ksrc Virtual source address in kernel space.
   * @param n Number of integral types to copy.
   */
  template < typename T >
  void copy_to_user (T *udst, T const *ksrc, size_t n);
};

//----------------------------------------------------------------------------
INTERFACE[ux]:

#include "entry_frame.h"

extern "C" FIASCO_FASTCALL
int
thread_page_fault (Address, Mword, Address, Mword, Return_frame *);

//----------------------------------------------------------------------------
IMPLEMENTATION:

#include <cassert>
#include "cpu.h"

IMPLEMENT inline
template < typename T >
T
Space::peek (T const *addr, bool user_space)
{
  // this == current_space() does not apply for SMAS
  return user_space ? peek_user (addr) : *addr;
}

IMPLEMENTATION[arm,ia32-!smas]:

IMPLEMENT inline NEEDS [<cassert>]
template < typename T >
T
Space::peek_user (T const *addr)
{
  assert (this == current_space());
  return *addr;
}

IMPLEMENT inline NEEDS [<cassert>]
template < typename T >
void
Space::poke_user (T *addr, T value)
{
  assert (this == current_space());
  *addr = value;
}

//----------------------------------------------------------------------------
IMPLEMENTATION[arm]:

IMPLEMENT inline NEEDS [<cassert>]
template < typename T >
void
Space::copy_from_user (T *kdst, T const *usrc, size_t n)
{
  assert (this == current_space());
  memcpy (kdst, usrc, n * sizeof (T));
}

IMPLEMENT inline NEEDS [<cassert>]
template < typename T >
void
Space::copy_to_user (T *udst, T const *ksrc, size_t n)
{
  assert (this == current_space());
  memcpy (udst, ksrc, n * sizeof (T));
}

//----------------------------------------------------------------------------
IMPLEMENTATION[ia32-!smas]:

IMPLEMENT inline NEEDS [<cassert>, "cpu.h"]
template < typename T >
void
Space::copy_from_user (T *kdst, T const *usrc, size_t n)
{
  assert (this == current_space());
  Cpu::memcpy_bytes (kdst, usrc, n * sizeof (T));
}

IMPLEMENT inline NEEDS [<cassert>, "cpu.h"]
template <>
void
Space::copy_from_user <Mword> (Mword *kdst, Mword const *usrc, size_t n)
{
  assert (this == current_space());
  Cpu::memcpy_mwords (kdst, usrc, n);
}

IMPLEMENT inline NEEDS [<cassert>, "cpu.h"]
template < typename T >
void
Space::copy_to_user (T *udst, T const *ksrc, size_t n)
{
  assert (this == current_space());
  Cpu::memcpy_bytes (udst, ksrc, n * sizeof (T));
}

IMPLEMENT inline NEEDS [<cassert>, "cpu.h"]
template <>
void
Space::copy_to_user <Mword> (Mword *udst, Mword const *ksrc, size_t n)
{
  assert (this == current_space());
  Cpu::memcpy_mwords (udst, ksrc, n);
}

//----------------------------------------------------------------------------
IMPLEMENTATION[ia32-smas]:

IMPLEMENT inline
template < typename T >
T
Space::peek_user (T const *addr)
{
  // this == current_space() does not apply for SMAS
  T ret;

  switch (sizeof (T))
    {
      case 1:
        asm ("movb %%fs:(%1), %b0" : "=r" (ret) : "Nr" (addr));
        break;
      case 2:
        asm ("movw %%fs:(%1), %w0" : "=r" (ret) : "Nr" (addr));
        break;
      case 4:
        asm ("movl %%fs:(%1),  %0" : "=r" (ret) : "Nr" (addr));
        break;
      default:
        copy_from_user < T > (&ret, addr, 1);
        break;
    }

  return ret;
}

IMPLEMENT inline
template < typename T >
void
Space::poke_user (T *addr, T value)
{
  // this == current_space() does not apply for SMAS
  switch (sizeof (T))
    {
      case 1:
        asm ("movb %b0, %%fs:(%1)" : : "Nr" (value), "Nr" (addr) : "memory");
        break;
      case 2:
        asm ("movw %w0, %%fs:(%1)" : : "Nr" (value), "Nr" (addr) : "memory");
        break;
      case 4:
        asm ("movl  %0, %%fs:(%1)" : : "Nr" (value), "Nr" (addr) : "memory");
        break;
      default:
        copy_to_user < T > (addr, &value, 1);
        break;
    }
}

IMPLEMENT inline NEEDS ["cpu.h"]
template < typename T >
void
Space::copy_from_user (T *kdst, T const *usrc, size_t n)
{
  // this == current_space() does not apply for SMAS 
  Cpu::memcpy_bytes_fs (kdst, usrc, n * sizeof (T));
}

IMPLEMENT inline NEEDS ["cpu.h"]
template <>
void
Space::copy_from_user <Mword> (Mword *kdst, Mword const *usrc, size_t n)
{
  // this == current_space() does not apply for SMAS
  Cpu::memcpy_mwords_fs (kdst, usrc, n);
}

IMPLEMENT inline NEEDS ["cpu.h"]
template < typename T >
void
Space::copy_to_user (T *udst, T const *ksrc, size_t n)
{
  // this == current_space() does not apply for SMAS
  asm ("mov %%fs, %%eax; mov %%eax, %%es" : : : "eax");
  Cpu::memcpy_bytes (udst, ksrc, n * sizeof (T));
  asm ("mov %%ds, %%eax; mov %%eax, %%es" : : : "eax");
}

IMPLEMENT inline NEEDS ["cpu.h"]
template <>
void
Space::copy_to_user <Mword> (Mword *udst, Mword const *ksrc, size_t n)
{
  // this == current_space() does not apply for SMAS
  asm ("mov %%fs, %%eax; mov %%eax, %%es" : : : "eax");
  Cpu::memcpy_mwords (udst, ksrc, n);
  asm ("mov %%ds, %%eax; mov %%eax, %%es" : : : "eax");
}

//----------------------------------------------------------------------------
IMPLEMENTATION[ux]:

#include "config.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "mem_layout.h"
#include "processor.h"
#include "regdefs.h"

/**
 * Translate virtual memory address in user space to virtual memory
 * address in kernel space (requires all physical memory be mapped).
 * @param addr Virtual address in user address space.
 * @param write Type of memory access (read or write) for page-faults.
 * @return Virtual address in kernel address space.
 */
PRIVATE
template < typename T >
T *
Space::user_to_kernel (T const *addr, bool write)
{
  Address phys, virt = (Address) addr;
  unsigned attr, error = 0;
  Address size;

  for (;;)
    {
      // See if there is a mapping for this address
      if (v_lookup (virt, &phys, &size, &attr))
        {
          // Add offset to frame
          phys |= (virt & ~(size == Config::SUPERPAGE_SIZE
                                  ? Config::SUPERPAGE_MASK
                                  : Config::PAGE_MASK));

          // See if we want to write and are not allowed to
          // Generic check because INTEL_PTE_WRITE == INTEL_PDE_WRITE
          if (!write || (attr & Pt_entry::Writable))
            return (T *) Mem_layout::phys_to_pmem (phys);

          error |= PF_ERR_PRESENT;
        }

      if (write)
        error |= PF_ERR_WRITE;

      // If we tried to access user memory of a space other than current_space()
      // our Long-IPC partner must do a page-in. This is analogue to IA32
      // page-faulting in the IPC window. Set PF_ERR_REMTADDR hint.
      // Otherwise we faulted on our own user memory. Set PF_ERR_USERADDR hint.
      error |= (this == current_space() ? PF_ERR_USERADDR : PF_ERR_REMTADDR);

      // No mapping or insufficient access rights, raise pagefault.
      // Pretend open interrupts, we restore the current state afterwards.
      Cpu_lock::Status was_locked = cpu_lock.test();

      thread_page_fault (virt, error, 0, 
			 Proc::processor_state() | EFLAGS_IF, 0);

      cpu_lock.set (was_locked);
    }
}

IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h"]
template < typename T >
T
Space::peek_user (T const *addr)
{
  T value;

  // Check if we cross page boundaries
  if (((Address) addr                   & Config::PAGE_MASK) ==
     (((Address) addr + sizeof (T) - 1) & Config::PAGE_MASK))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      value = *user_to_kernel (addr, false);
    }
  else
    copy_from_user < T > (&value, addr, 1);

  return value;
}

IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h"]
template < typename T >
void
Space::poke_user (T *addr, T value)
{
  // Check if we cross page boundaries
  if (((Address) addr                   & Config::PAGE_MASK) ==
     (((Address) addr + sizeof (T) - 1) & Config::PAGE_MASK))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);
      *user_to_kernel (addr, true) = value;
    }
  else
    copy_to_user < T > (addr, &value, 1);
}

IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h"]
template < typename T >
void
Space::copy_from_user (T *kdst, T const *usrc, size_t n)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  char *ptr = (char *) usrc;
  char *dst = (char *) kdst;
  char *src = 0;

  n *= sizeof (T);

  while (n--)
    {
      if (!src || ((Address) ptr & ~Config::PAGE_MASK) == 0)
        src = user_to_kernel (ptr, false);
        
      *dst++ = *src++; ptr++;
    }
}

IMPLEMENT inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h"]
template < typename T >
void
Space::copy_to_user (T *udst, T const *ksrc, size_t n)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  char *ptr = (char *) udst;
  char *src = (char *) ksrc;
  char *dst = 0;

  n *= sizeof (T);

  while (n--)
    {
      if (!dst || ((Address) ptr & ~Config::PAGE_MASK) == 0)
        dst = user_to_kernel (ptr, true);

      *dst++ = *src++; ptr++;
    }
}

/**
 * Copy integral types between two user address spaces.
 * @param dst Destination address space.
 * @param udst Virtual destination address in dst's user space.
 * @param usrc Virtual source address in this user space.
 * @param n Number of integral types to copy.
 */
PUBLIC inline NEEDS ["config.h", "cpu_lock.h", "lock_guard.h"]
template < typename T >
void
Space::copy_user_to_user (Space *dst, T *udst, T *usrc, size_t n)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  char *src_uvirt = (char *) usrc;
  char *dst_uvirt = (char *) udst;
  char *src_kvirt = 0;
  char *dst_kvirt = 0;

  n *= sizeof (T);

  while (n--)
    {
      if (!src_kvirt || ((Address) src_uvirt & ~Config::PAGE_MASK) == 0)
        src_kvirt = user_to_kernel (src_uvirt, false);
      if (!dst_kvirt || ((Address) dst_uvirt & ~Config::PAGE_MASK) == 0)
        dst_kvirt = dst->user_to_kernel (dst_uvirt, true);

      *dst_kvirt++ = *src_kvirt++; src_uvirt++; dst_uvirt++;
    }
}
