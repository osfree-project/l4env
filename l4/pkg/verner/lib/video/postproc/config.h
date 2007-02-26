
/* Runtime CPU detection */
#undef RUNTIME_CPUDETECT
#define RUNTIME_CPUDETECT 1

/* "restrict" keyword */
#define restrict __restrict

/* Define this if your system has the "malloc.h" header file */
#define HAVE_MALLOC_H 1

/* memalign is mapped to malloc if unsupported */
#define HAVE_MEMALIGN 1
#ifndef HAVE_MEMALIGN
# define memalign(a,b) malloc(b)
#endif

/* Define this if your system has the "alloca.h" header file */
#define HAVE_ALLOCA_H 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#undef WORDS_BIGENDIAN

#define ARCH_X86 1


/* Define this to any prefered value from 386 up to infinity with step 100 */
#define __CPU__ 686

#define MP_WORDSIZE 32

#define TARGET_LINUX 1
#undef TARGET_LINUX



/* Extension defines */
#define HAVE_3DNOW 1	// only define if you have 3DNOW (AMD k6-2, AMD Athlon, iDT WinChip, etc.)
#define HAVE_3DNOWEX 1	// only define if you have 3DNOWEX (AMD Athlon, etc.)
#define HAVE_MMX 1	// only define if you have MMX (newer x86 chips, not P54C/PPro)
#define HAVE_MMX2 1	// only define if you have MMX2 (Athlon/PIII/4/CelII)
#define HAVE_SSE 1	// only define if you have SSE (Intel Pentium III/4 or Celeron II)
#undef HAVE_SSE2	// only define if you have SSE2 (Intel Pentium 4)
#undef HAVE_ALTIVEC	// only define if you have Altivec (G4)

#ifdef HAVE_MMX
#define USE_MMX_IDCT 1
#endif

