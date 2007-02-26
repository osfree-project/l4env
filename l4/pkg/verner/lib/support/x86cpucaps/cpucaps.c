/*
 *
 * 'cpucaps', sample code for libx86cpucaps
 * by Osamu Kayasono <jacobi@jcom.home.ne.jp>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "x86cpucaps.h"

#define OUT_ALL       0
#define OUT_KERNELOPT 1
#define OUT_GCCTARGET 2
#define OUT_GCCSIMD   3
#define OUT_GCCALL    4

struct x86cpucaps cpucaps;
struct simdcaps sdcaps;   

#define DEBUG 1 // set to 1, verbose

/* internal prototypes */
static void print_simdcaps(int i);
static void print_cpuinfo(int outmode);


static void print_simdcaps(int i)
{
   switch (i) {
    case HAS_SSE2:
      printf("SSE2\n");
      break;
    case HAS_SSE:
      printf("SSE\n");
      break;
    case HAS_3DNOWEXT:
      printf("3DNow! extensions\n");
      break;
    case HAS_3DNOW:
      printf("3DNow!\n");
      break;
/*    case HAS_MMXEXT:
      printf("MMX extensions\n");
      break; */
    case HAS_MMX:
      printf("MMX\n");
      break;
    default:
      printf("none\n");
   }
   return;
}

static void print_cpuinfo(int outmode)
{
   switch (outmode) 
     {
      case OUT_KERNELOPT:
	printf("%s\n", cpucaps.kernelopt);
	break;
      case OUT_GCCTARGET:
      case OUT_GCCSIMD:
      case OUT_GCCALL:
	if (outmode != OUT_GCCSIMD ) printf("%s ", cpucaps.gcctarget);
	if (outmode != OUT_GCCTARGET ) printf("%s", cpucaps.gccsimdopt);
	printf("\n");
	break;
      default:
	printf("\n");
	printf("CPU info                           : %d - %d - %d (%s)\n",
	       cpucaps.cpu_family, cpucaps.cpu_model, cpucaps.cpu_stepping,
	       cpucaps.vendor_name);
	printf("CPU Model Name                     : %s\n", cpucaps.cpu_name);
	//printf("Recommended Kernel building option : %s\n", cpucaps.kernelopt);
	//printf("Recommended gcc (%.4f) target    : %s %s\n", cpucaps.gccver, cpucaps.gcctarget, cpucaps.gccsimdopt);

	printf("checking Intel SIMD capability     : ");
	print_simdcaps(cpucaps.intel_simd);

	printf("checking AMD 3DNow! capability     : ");
	print_simdcaps(cpucaps.amd_simd);
#ifndef _WIN32_
//	if (sdcaps.has_sse == TRUE) 
//	     sdcaps.has_sse= x86cpucaps_check_sse_supported(sdcaps.has_sse,DEBUG);
#endif
	
	printf("SIMD capabilities checking results\n");
	printf("   SSE2:%d, SSE:%d, MMXext:%d, MMX:%d,  3DNow!Ex:%d, 3DNow!:%d\n",
	     sdcaps.has_sse2,sdcaps.has_sse,sdcaps.has_mmxext, sdcaps.has_mmx,
	     sdcaps.has_3dnowext,sdcaps.has_3dnow);

     }
   
   return;
}


int x86cpucaps_getdesc(int verbose)
{
   int outmode = OUT_ALL;
   int cpu_id;
   char *tmpstr;

   cpucaps.vendor_id = x86cpucaps_vendor(cpucaps.vendor_name);
   
   cpucaps.cpu_family   = x86cpucaps_cpumodel(GET_FAMILY);
   cpucaps.cpu_model    = x86cpucaps_cpumodel(GET_MODEL);
   cpucaps.cpu_stepping = x86cpucaps_cpumodel(GET_STEPPING); 
/*   cpu_id = x86cpucaps_cpumodel(GET_ALLID); */
   cpu_id = (cpucaps.cpu_family << 8) + (cpucaps.cpu_model << 4) + cpucaps.cpu_stepping;

   tmpstr =  x86cpucaps_getcpuname(cpucaps.vendor_id, cpu_id);
   strncpy(cpucaps.cpu_name, tmpstr, LEN_CPUNAME);
   free(tmpstr);

   tmpstr = x86cpucaps_getkernelopt(cpucaps.vendor_id, cpu_id);
   strncpy(cpucaps.kernelopt, tmpstr, LEN_KERNELOPT);
   free(tmpstr);
   
   tmpstr = x86cpucaps_getgcctarget(cpucaps.gccver, cpucaps.vendor_id, cpu_id);
   strncpy(cpucaps.gcctarget, tmpstr, LEN_GCCTARGET);
   free(tmpstr);
   
   tmpstr = x86cpucaps_getgccsimdopt(cpucaps.gccver, cpucaps.vendor_id, cpu_id);
   strncpy(cpucaps.gccsimdopt, tmpstr, LEN_GCCSIMDOPT);
   free(tmpstr);

   cpucaps.intel_simd = x86cpucaps_simd(GET_INTELSIMD); 
   cpucaps.amd_simd = x86cpucaps_simd(GET_AMDSIMD);

   x86cpucaps_simdall(&sdcaps,0);
   
   if(verbose) print_cpuinfo(outmode);

   /* cr7: build an XviD compatible int */
   return (sdcaps.has_mmx << 0 |  sdcaps.has_mmxext << 1 | sdcaps.has_sse << 2 |  sdcaps.has_sse2 << 3 | sdcaps.has_3dnow << 4 |sdcaps.has_3dnowext  << 5);
}
