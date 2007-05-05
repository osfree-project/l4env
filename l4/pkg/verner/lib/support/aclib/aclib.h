#define ARCH_X86          1
#define TARGET_ARCH_X86   1
#define TARGET_SSE        1
#define RUNTIME_CPUDETECT 1
#define USE_FAKE_MONO     1
#define USE_FASTMEMCPY    1

#define HAVE_3DNOW       1 
#define HAVE_3DNOWEX     1
#define HAVE_MMX         1
#define HAVE_MMX2        1
#define HAVE_SSE         1
#define HAVE_SSE2        1

/*
  aclib - advanced C library ;)
  This file contains functions which improve and expand standard C-library
*/

#define BLOCK_SIZE 4096
#define CONFUSION_FACTOR 0

#ifndef HAVE_SSE2
/*
   P3 processor has only one SSE decoder so can execute only 1 sse insn per
   cpu clock, but it has 3 mmx decoders (include load/store unit)
   and executes 3 mmx insns per cpu clock.
   P4 processor has some chances, but after reading:
   http://www.emulators.com/pentium4.htm
   I have doubts. Anyway SSE2 version of this code can be written better.
*/
#undef HAVE_SSE
#endif


/*
 This part of code was taken by me from Linux-2.4.3 and slightly modified
for MMX, MMX2, SSE instruction set. I have done it since linux uses page aligned
blocks but mplayer uses weakly ordered data and original sources can not
speedup them. Only using PREFETCHNTA and MOVNTQ together have effect!

>From IA-32 Intel Architecture Software Developer's Manual Volume 1,

Order Number 245470:
"10.4.6. Cacheability Control, Prefetch, and Memory Ordering Instructions"

Data referenced by a program can be temporal (data will be used again) or
non-temporal (data will be referenced once and not reused in the immediate
future). To make efficient use of the processor's caches, it is generally
desirable to cache temporal data and not cache non-temporal data. Overloading
the processor's caches with non-temporal data is sometimes referred to as
"polluting the caches".
The non-temporal data is written to memory with Write-Combining semantics.

The PREFETCHh instructions permits a program to load data into the processor
at a suggested cache level, so that it is closer to the processors load and
store unit when it is needed. If the data is already present in a level of
the cache hierarchy that is closer to the processor, the PREFETCHh instruction
will not result in any data movement.
But we should you PREFETCHNTA: Non-temporal data fetch data into location
close to the processor, minimizing cache pollution.

The MOVNTQ (store quadword using non-temporal hint) instruction stores
packed integer data from an MMX register to memory, using a non-temporal hint.
The MOVNTPS (store packed single-precision floating-point values using
non-temporal hint) instruction stores packed floating-point data from an
XMM register to memory, using a non-temporal hint.

The SFENCE (Store Fence) instruction controls write ordering by creating a
fence for memory store operations. This instruction guarantees that the results
of every store instruction that precedes the store fence in program order is
globally visible before any store instruction that follows the fence. The
SFENCE instruction provides an efficient way of ensuring ordering between
procedures that produce weakly-ordered data and procedures that consume that
data.

If you have questions please contact with me: Nick Kurshev: nickols_k@mail.ru.
*/

// 3dnow memcpy support from kernel 2.4.2
//  by Pontscho/fresh!mindworkz


#undef HAVE_MMX1
#if defined(HAVE_MMX) && !defined(HAVE_MMX2) && !defined(HAVE_3DNOW) && !defined(HAVE_SSE)
/*  means: mmx v.1. Note: Since we added alignment of destinition it speedups
    of memory copying on PentMMX, Celeron-1 and P2 upto 12% versus
    standard (non MMX-optimized) version.
    Note: on K6-2+ it speedups memory copying upto 25% and
          on K7 and P3 about 500% (5 times). */
#define HAVE_MMX1
#endif


#undef HAVE_K6_2PLUS
#if !defined( HAVE_MMX2) && defined( HAVE_3DNOW)
#define HAVE_K6_2PLUS
#endif

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(to,from,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"rep; movsb"\
	:"=&D"(to), "=&S"(from), "=&c"(dummy)\
/* It's most portable way to notify compiler */\
/* that edi, esi and ecx are clobbered in asm block. */\
/* Thanks to A'rpi for hint!!! */\
        :"0" (to), "1" (from),"2" (n)\
	: "memory");\
}

#undef MMREG_SIZE
#ifdef HAVE_SSE
#define MMREG_SIZE 16
#else
#define MMREG_SIZE 64 //8
#endif

#undef PREFETCH
#undef EMMS
#define EMMS     "emms"

#ifdef HAVE_MMX2
#define PREFETCH "prefetchnta"
#elif defined ( HAVE_3DNOW )
#define PREFETCH  "prefetch"
#else
#define PREFETCH "/nop"
#endif


#undef MOVNTQ
#ifdef HAVE_MMX2
#define MOVNTQ "movntq"
#else
#define MOVNTQ "movq"
#endif

#undef MIN_LEN
#ifdef HAVE_MMX1
#define MIN_LEN 0x800  /* 2K blocks */
#else
#define MIN_LEN 0x40  /* 64-byte blocks */
#endif

//cr7
//#define size_t unsigned long
#define __need_size_t
#include <stddef.h>
extern void * fast_memcpy_MMX(void * to, const void * from, size_t len);
extern void * fast_memcpy_MMX2(void * to, const void * from, size_t len);
extern void * fast_memcpy_3DNow(void * to, const void * from, size_t len);
extern void * fast_memcpy_3DNow_SSE(void * to, const void * from, size_t len);
extern void * fast_memcpy_SSE(void * to, const void * from, size_t len);

//cr7
extern void * determineFastMemcpy(int cpucaps);
