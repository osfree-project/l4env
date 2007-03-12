INTERFACE:

#include "std_macros.h"

EXTENSION class Mmu
{
private:
  enum 
  {
    Cacheline_size = 32,
  };
};


// ---------------------------------------------------------------------------
IMPLEMENTATION [arm && (pxa || sa1100 || 926)]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const * /*start*/, 
				       void const * /*end*/)
{
  flush_cache();
}

IMPLEMENT 
template< unsigned long Flush_area , bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
  if (((Address)end) - ((Address)start) >= 8192)
    clean_dcache();
  else
    {
      asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c10, 1 \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  "    mcr  p15, 0, %0, c7, c10, 4 \n" // drain WB
	  : : "r" (start), "r" (end), "i" (Cacheline_size)
	  );
    }
}

IMPLEMENT
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
#if 1
  __asm__ __volatile__ (
      "mcr p15, 0, %0, c7, c10, 1       \n"
      : : "r"(va) : "memory");
#endif
  //clean_dcache();
}

IMPLEMENT 
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
  if (((Address)end) - ((Address)start) >= 8192)
    flush_dcache();
  else
    {
      asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c14, 1 \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  "    mcr  p15, 0, %0, c7, c10, 4 \n" // drain WB
	  : : "r" (start), "r" (end), "i" (Cacheline_size)
	  );
    }
}


IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c6, 1  \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  : : "r" (start), "r" (end), "i" (Cacheline_size)
	  );
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && mpcore]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const *start,
				       void const *end)
{
#if 1
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "1:  mcr p15, 0, %0, c7, c14, 1       \n"
      "    mcr p15, 0, %0, c7, c5, 1        \n"
      "    add %0, %0, #4                   \n"
      "    cmp %0, %1                       \n"
      "    blo 1b                           \n"
      : "=r" (start) : "0"(start), "r"(end)
      : "r0", "memory");
#endif
//  flush_cache();
}

IMPLEMENT inline
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
#if 1
  __asm__ __volatile__ (
      "mcr p15, 0, %0, c7, c10, 1       \n"
      : : "r"(va) : "memory");
#endif
  //clean_dcache();
}

IMPLEMENT inline
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
#if 1
  __asm__ __volatile__ (
      "    mov %0, %1                       \n"
      "    mcr p15, 0, %0, c7, c10, 4       \n"
      "1:  mcr p15, 0, %0, c7, c10, 1       \n"
      "    add %0, %0, #4                   \n"
      "    cmp %0, %2                       \n"
      "    blo 1b                           \n"
      : "=&r" (start) : "r"(start), "r"(end)
      : "memory");
#endif
  //clean_dcache();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
#if 1
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "1:  mcr p15, 0, %0, c7, c14, 1       \n"
      "    add %0, %0, #4                   \n"
      "    cmp %0, %1                       \n"
      "    blo 1b                           \n"
      : "=r" (start) : "0"(start), "r"(end)
      : "r0", "memory");
#endif
  //flush_dcache();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "1:  mcr p15, 0, %0, c7, c6, 1        \n"
      "    add %0, %0, #4                   \n"
      "    cmp %0, %1                       \n"
      "    blo 1b                           \n"
      : "=r" (start) : "0"(start), "r"(end)
      : "r0", "memory");
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache()
{
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "    mcr p15, 0, r0, c7, c14, 0       \n"
      "    mcr p15, 0, r0, c7, c5, 0        \n"
      : : : "memory");
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache()
{
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "    mcr p15, 0, r0, c7, c10, 0       \n"
      : : : "memory");
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache()
{
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 4       \n"
      "    mcr p15, 0, r0, c7, c14, 0       \n"
      : : : "memory");
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && sa1100]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192           \n" // 8k flush area
      " 1:  ldr r0, [%1], %2            \n" // 32 bytes cache line size
      "     teq %1, %0                  \n"
      "     bne 1b                      \n"
      "     mov r0, #0                  \n"
      "     mcr  p15, 0, r0, c7, c7, 0  \n"
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cacheline_size)
      : "r0"
      );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache() 
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192 \n" // 8k flush area
      " 1:  ldr r0, [%1], %2  \n" // 32 bytes cache line size
      "     teq %1, %0        \n"
      "     bne 1b            \n"
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cacheline_size)
      : "r0"
      );
}

IMPLEMENT 
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192           \n" // 8k flush area
      " 1:  ldr r0, [%1], %2            \n" // 32 bytes cache line size
      "     teq %1, %0                  \n"
      "     bne 1b                      \n"
      "     mov  r0, #0                 \n"
      "     mcr  p15, 0, r0, c7, c6, 0  \n" // inv D cache
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cacheline_size)
      : "r0"
      );

}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && pxa]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  register Mword dummy1, dummy2;
  asm volatile 
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr  p15, 0, %0, c7, c7, 0  \n"
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t" 
     :
     "=r" (dummy1),
     "=r" (dummy2)
     : 
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  register Mword dummy1, dummy2;
  asm volatile 
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t" 
     :
     "=r" (dummy1),
     "=r" (dummy2)
     : 
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  register Mword dummy1, dummy2;
  asm volatile 
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     "    mcr  p15, 0, %0, c7, c6, 0  \n" // inv D cache
     // drain write buffer
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t" 
     :
     "=r" (dummy1),
     "=r" (dummy2)
     : 
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && 926]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  asm volatile 
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, %0, c7, c7, 0  \n"
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t" 
     : : 
     "r" (0)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  asm volatile 
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t" 
     : : 
     "r" (0)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  asm volatile 
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     "    mcr  p15, 0, %0, c7, c6, 0  \n" // inv D cache
     // drain write buffer
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t" 
     : :
     "r" (0)
    );
}


