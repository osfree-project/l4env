#include <stddef.h>
#include "aclib.h"

#define RENAME(a) a ## _MMX
#undef HAVE_SSE
#undef HAVE_SSE2
/*
 * with MMX and without SSE(2)
 */
void * RENAME(fast_memcpy)(void * to, const void * from, size_t len)
{
	void *retval;
	size_t i;
	retval = to;
#ifdef STATISTICS
	{
		static int freq[33];
		static int t=0;
		int i;
		for(i=0; len>(1<<i); i++);
		freq[i]++;
		t++;
		if(1024*1024*1024 % t == 0)
			for(i=0; i<32; i++)
				printf("freq < %8d %4d\n", 1<<i, freq[i]);
	}
#endif
#ifndef HAVE_MMX1
        /* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
	        PREFETCH" (%0)\n"
	        PREFETCH" 64(%0)\n"
	        PREFETCH" 128(%0)\n"
        	PREFETCH" 192(%0)\n"
        	PREFETCH" 256(%0)\n"
		: : "r" (from) );
#endif
        if(len >= MIN_LEN)
	{
	  register unsigned long int delta;
          /* Align destinition to MMREG_SIZE -boundary */
          delta = ((unsigned long int)to)&(MMREG_SIZE-1);
          if(delta)
	  {
	    delta=MMREG_SIZE-delta;
	    len -= delta;
	    small_memcpy(to, from, delta);
	  }
	  i = len >> 6; /* len/64 */
	  len&=63;
        /*
           This algorithm is top effective when the code consequently
           reads and writes blocks which have size of cache line.
           Size of cache line is processor-dependent.
           It will, however, be a minimum of 32 bytes on any processors.
           It would be better to have a number of instructions which
           perform reading and writing to be multiple to a number of
           processor's decoders, but it's not always possible.
        */
#ifdef HAVE_SSE /* Only P3 (may be Cyrix3) */
	if(((unsigned long)from) & 15)
	/* if SRC is misaligned */
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		PREFETCH" 320(%0)\n"
		"movups (%0), %%xmm0\n"
		"movups 16(%0), %%xmm1\n"
		"movups 32(%0), %%xmm2\n"
		"movups 48(%0), %%xmm3\n"
		"movntps %%xmm0, (%1)\n"
		"movntps %%xmm1, 16(%1)\n"
		"movntps %%xmm2, 32(%1)\n"
		"movntps %%xmm3, 48(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}
	else
	/*
	   Only if SRC is aligned on 16-byte boundary.
	   It allows to use movaps instead of movups, which required data
	   to be aligned or a general-protection exception (#GP) is generated.
	*/
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		PREFETCH" 320(%0)\n"
		"movaps (%0), %%xmm0\n"
		"movaps 16(%0), %%xmm1\n"
		"movaps 32(%0), %%xmm2\n"
		"movaps 48(%0), %%xmm3\n"
		"movntps %%xmm0, (%1)\n"
		"movntps %%xmm1, 16(%1)\n"
		"movntps %%xmm2, 32(%1)\n"
		"movntps %%xmm3, 48(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}
#else
	// Align destination at BLOCK_SIZE boundary
	for(; ((int)to & (BLOCK_SIZE-1)) && i>0; i--)
	{
		__asm__ __volatile__ (
#ifndef HAVE_MMX1
        	PREFETCH" 320(%0)\n"
#endif
		"movq (%0), %%mm0\n"
		"movq 8(%0), %%mm1\n"
		"movq 16(%0), %%mm2\n"
		"movq 24(%0), %%mm3\n"
		"movq 32(%0), %%mm4\n"
		"movq 40(%0), %%mm5\n"
		"movq 48(%0), %%mm6\n"
		"movq 56(%0), %%mm7\n"
		MOVNTQ" %%mm0, (%1)\n"
		MOVNTQ" %%mm1, 8(%1)\n"
		MOVNTQ" %%mm2, 16(%1)\n"
		MOVNTQ" %%mm3, 24(%1)\n"
		MOVNTQ" %%mm4, 32(%1)\n"
		MOVNTQ" %%mm5, 40(%1)\n"
		MOVNTQ" %%mm6, 48(%1)\n"
		MOVNTQ" %%mm7, 56(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}

//	printf(" %d %d\n", (int)from&1023, (int)to&1023);
	// Pure Assembly cuz gcc is a bit unpredictable ;)
	if(i>=BLOCK_SIZE/64)
		asm volatile(
			"xorl %%eax, %%eax	\n\t"
			".balign 16		\n\t"
			"1:			\n\t"
				"movl (%0, %%eax), %%ebx 	\n\t"
				"movl 32(%0, %%eax), %%ebx 	\n\t"
				"movl 64(%0, %%eax), %%ebx 	\n\t"
				"movl 96(%0, %%eax), %%ebx 	\n\t"
				"addl $128, %%eax		\n\t"
				"cmpl %3, %%eax			\n\t"
				" jb 1b				\n\t"

			"xorl %%eax, %%eax	\n\t"

				".balign 16		\n\t"
				"2:			\n\t"
				"movq (%0, %%eax), %%mm0\n"
				"movq 8(%0, %%eax), %%mm1\n"
				"movq 16(%0, %%eax), %%mm2\n"
				"movq 24(%0, %%eax), %%mm3\n"
				"movq 32(%0, %%eax), %%mm4\n"
				"movq 40(%0, %%eax), %%mm5\n"
				"movq 48(%0, %%eax), %%mm6\n"
				"movq 56(%0, %%eax), %%mm7\n"
				MOVNTQ" %%mm0, (%1, %%eax)\n"
				MOVNTQ" %%mm1, 8(%1, %%eax)\n"
				MOVNTQ" %%mm2, 16(%1, %%eax)\n"
				MOVNTQ" %%mm3, 24(%1, %%eax)\n"
				MOVNTQ" %%mm4, 32(%1, %%eax)\n"
				MOVNTQ" %%mm5, 40(%1, %%eax)\n"
				MOVNTQ" %%mm6, 48(%1, %%eax)\n"
				MOVNTQ" %%mm7, 56(%1, %%eax)\n"
				"addl $64, %%eax		\n\t"
				"cmpl %3, %%eax		\n\t"
				"jb 2b				\n\t"

#if CONFUSION_FACTOR > 0
	// a few percent speedup on out of order executing CPUs
			"movl %5, %%eax		\n\t"
				"2:			\n\t"
				"movl (%0), %%ebx	\n\t"
				"movl (%0), %%ebx	\n\t"
				"movl (%0), %%ebx	\n\t"
				"movl (%0), %%ebx	\n\t"
				"decl %%eax		\n\t"
				" jnz 2b		\n\t"
#endif

			"xorl %%eax, %%eax	\n\t"
			"addl %3, %0		\n\t"
			"addl %3, %1		\n\t"
			"subl %4, %2		\n\t"
			"cmpl %4, %2		\n\t"
			" jae 1b		\n\t"
				: "+r" (from), "+r" (to), "+r" (i)
				: "r" (BLOCK_SIZE), "i" (BLOCK_SIZE/64), "i" (CONFUSION_FACTOR)
				: "%eax", "%ebx"
		);

	for(; i>0; i--)
	{
		__asm__ __volatile__ (
#ifndef HAVE_MMX1
        	PREFETCH" 320(%0)\n"
#endif
		"movq (%0), %%mm0\n"
		"movq 8(%0), %%mm1\n"
		"movq 16(%0), %%mm2\n"
		"movq 24(%0), %%mm3\n"
		"movq 32(%0), %%mm4\n"
		"movq 40(%0), %%mm5\n"
		"movq 48(%0), %%mm6\n"
		"movq 56(%0), %%mm7\n"
		MOVNTQ" %%mm0, (%1)\n"
		MOVNTQ" %%mm1, 8(%1)\n"
		MOVNTQ" %%mm2, 16(%1)\n"
		MOVNTQ" %%mm3, 24(%1)\n"
		MOVNTQ" %%mm4, 32(%1)\n"
		MOVNTQ" %%mm5, 40(%1)\n"
		MOVNTQ" %%mm6, 48(%1)\n"
		MOVNTQ" %%mm7, 56(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}

#endif /* Have SSE */
#ifdef HAVE_MMX2
                /* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
#endif
#ifndef HAVE_SSE
		/* enables to use FPU */
		__asm__ __volatile__ (EMMS:::"memory");
#endif
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memcpy(to, from, len);
	return retval;
}

