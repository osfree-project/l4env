/* 
 * $Id$
 */

/*****************************************************************************
 * libl4util/src/micros2l4to.c                                               *
 * calculate L4 timeout                                                      *
 *****************************************************************************/

#include <stdio.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/util/bitops.h>

int
micros2l4to(int mus, int *to_e, int *to_m)
{
  if (mus <= 0)
    {
      *to_e = 1;
      *to_m = 0;
    }
  else
    {
      *to_e = 14 - l4util_log2(mus / 256) / 2;
      *to_m = mus / (1UL << (2 * (15 - *to_e)));
      
      if ((*to_e < 0) || (*to_e > 15) || (*to_m < 0) || (*to_m > 255))
        {
	  printf("micros2l4to(): invalid timeout %d, using max. values", mus);
	  *to_e = 0;
	  *to_m = 255;
        }
    }
  return 0;
}
