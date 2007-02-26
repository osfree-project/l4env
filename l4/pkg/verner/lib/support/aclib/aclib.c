#include "aclib.h"

/* OSKit includes */
#include <stdio.h>	/* printf */
#include <string.h>	/* memcpy */

void * determineFastMemcpy(int cpucaps)
{
  /* 
   * P3: SSE  --> memcpy_MMX2 (only one SSE execution unit)
   * P4: SSE2 --> memcpy_SSE
   * Athlon/Duron: 3dnow --> memcpy_3dnow
   * Athlon XP:    SSE   --> memcpy_3dnow+SSE
   * Athlon 64     SSE2  --> memcpy_3dnow+SSE
   */
  if ((cpucaps & (1 << 5)) || (cpucaps & (1 << 4))) /* CPU_3DNOW ext (1<< 5) || CPU_3DNOW (1<< 4) */
  {
    printf (" using 3DNow! optimized fastmemcpy\n");
    if (cpucaps & (1 << 2)) /* CPU_SSE (1<< 2) */
    /* w/ sse */
	return fast_memcpy_3DNow_SSE;
    else
    /* w/o sse */
	return fast_memcpy_3DNow;
  }
  else if (cpucaps & (1 << 2))	/* CPU_SSE (1<< 2) */
  {
    if ((cpucaps & (1 << 3)))	/* CPU_SSE2 */
    {
      printf ("using SSE optimized fastmemcpy\n");
      return fast_memcpy_SSE;
    }
    else
    {
      printf ("using MMX2 optimized fastmemcpy\n");
      return fast_memcpy_MMX2;
    }
  }
  else if (cpucaps & (1 << 0))	/* CPU_MMX (1<< 0) */
  {
    printf ("using MMX optimized fastmemcpy\n");
    return fast_memcpy_MMX;
  }
  printf ("using generic memcpy\n");
  return memcpy;
}   
